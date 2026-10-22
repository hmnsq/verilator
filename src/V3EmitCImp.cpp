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
    ध्रुव Ast_ग्रन्थिः_Module* ध्रुव m_fileModp;
    ध्रुव तार्किक m_slow;  // Creating __Slow file
    V3_Unique_नामानि m_unique_नामानि;  // Generates unique file names
    ध्रुव std::string m_file_आधारः_Name = EmitCUtil::उपसर्गः_Name_Protect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctor_विभज_Decl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtor_विभज_Decl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    शून्य open_अग्र_Output_File(ध्रुव std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, असत्य, "Design implementation internals");
        emit_File_शीर्षकम्();
    }
    शून्य emit_File_शीर्षकम्() {
        पङ्क्तिं_लिख("// See " + EmitCUtil::शिखरम्_Class_Name() + ".h for the primary calling header\n");
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    शून्य emitParamDefns(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    यदि (first) {
                        पङ्क्तिं_लिख("\n");
                        ध्रुव string modName = EmitCUtil::उपसर्गः_Name_Protect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = असत्य;
                    }
                    यदि (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    पङ्क्तिं_लिख(";\n");
                }
            }
        }
        यदि (!first) पङ्क्तिं_लिख("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    स्थिर शून्य सङ्कलय_Var_Ctor_Dtor_Stmts(TextStmts& stmts, ध्रुव AstVar* varp, तार्किक destroy) {
        ध्रुव std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        यदि (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::सदिशम्<std::string> elems{name};
            प्रति (ध्रुव AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::सदिशम्<std::string> subElems;
                प्रति (ध्रुव std::string& elem : elems) {
                    प्रति (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.प्रक्षिप_पृष्ठभागः(elem + "[" + std::गन्तव्यम्_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            यदि (destroy) std::reverse(elems.begin(), elems.अन्तः_2());
            प्रति (ध्रुव std::string& e : elems) {
                stmts.सङ्कलय((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            प्रत्यागम;
        }

        // Other variables are constructed whole, even if aggregates
        यदि (destroy) {
            stmts.सङ्कलय("VL_INPLACE_DESTRUCT(" + name + ");");
            प्रत्यागम;
        }
        ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string तर्काः;
        यदि (varp->isSc()) {
            // SystemC ports need their name
            तर्काः = ", \"" + V3_बहिर्गमनम्_Formatter::quoteNameControls(varp->nameProtect()) + "\"";
        } अन्यथा यदि (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            तर्काः = ", " + VN_AS(varp->valuep(), Const)->अङ्कः().emitC();
        } अन्यथा यदि (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            तर्काः = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.सङ्कलय("VL_INPLACE_CONSTRUCT(" + name + तर्काः + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    शून्य createCtorDtorStmts(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Construct member variables
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                सङ्कलय_Var_Ctor_Dtor_Stmts(m_ctorStmts, varp, असत्य);
            }
        }

        // Destroy in reverse order
        ध्रुव Ast_ग्रन्थिः* ध्रुव lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : शून्यसूचकः;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                सङ्कलय_Var_Ctor_Dtor_Stmts(m_dtorStmts, varp, सत्य);
            }
        }

        // Split into sub-functions if needed
        ध्रुव std::string modName = EmitCUtil::उपसर्गः_Name_Protect(modp);
        std::tie(m_ctor_विभज_Decl, m_dtor_विभज_Decl) = emit_विभज_Ctor_Dtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", असत्य,
            "Design implementation internals", [एतत्]() { emit_File_शीर्षकम्(); });
    }
    शून्य emitCtorDtorImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव std::string modName = EmitCUtil::उपसर्गः_Name_Protect(modp);

        // ctor
        यदि (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        यदि (!m_ctor_विभज_Decl.रिक्तानि()) पङ्क्तिं_लिख("\n" + m_ctor_विभज_Decl + "\n");
        पङ्क्तिं_लिख("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        पङ्क्तिं_लिख("vlSelf->vlSymsp = symsp;\n");
        यदि (modp->is_शिखरम्()) {
            पङ्क्तिं_लिख("vlSelf->vlNamep = strdup(namep);\n");
        } अन्यथा {
            पङ्क्तिं_लिख("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        प्रति (ध्रुव TextStmts::Stmt& stmt : m_ctorStmts) पङ्क्तिं_लिख(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        यदि (modp->ctorVarReset()) {
            पङ्क्तिं_लिख(modName + "__" + protect("_ctor_var_reset") + "(vlSelf);\n");
        }
        पङ्क्तिं_लिख("}\n");

        // C++ constructor
        पङ्क्तिं_लिख("\n" + modName + "::" + modName + "(" + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        पङ्क्तिं_लिख(modName + "__ctor(this, symsp, namep);\n");
        emitSystemCSection(modp, VSystemCSectionType::CTOR);
        पङ्क्तिं_लिख("}\n");

        // dtor
        यदि (!m_dtor_विभज_Decl.रिक्तानि()) पङ्क्तिं_लिख("\n" + m_dtor_विभज_Decl + "\n");
        पङ्क्तिं_लिख("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        प्रति (ध्रुव TextStmts::Stmt& stmt : m_dtorStmts) पङ्क्तिं_लिख(stmt.m_text + "\n");
        पङ्क्तिं_लिख("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        पङ्क्तिं_लिख("}\n");

        // C++ destructor
        पङ्क्तिं_लिख("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        पङ्क्तिं_लिख(modName + "__dtor(this);\n");
        पङ्क्तिं_लिख("}\n");
    }

    शून्य emitConfigureImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव string modName = EmitCUtil::उपसर्गः_Name_Protect(modp);

        यदि (v3Global.opt.coverage()) {
            पङ्क्तिं_लिख("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        यदि (v3Global.opt.coverage()) {
            पङ्क्तिं_लिख("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            पङ्क्तिं_लिख("(void)first;  // Prevent unused variable warning\n");
            पङ्क्तिं_लिख(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            पङ्क्तिं_लिख("}\n");
        }
    }
    शून्य emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        यदि (v3Global.opt.coverage()) {
            पङ्क्तिं_लिख("\n// Coverage\n");
            पङ्क्तिं_लिख("void " + EmitCUtil::उपसर्गः_Name_Protect(m_modp) + "::__vlCoverInsert(");
            पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            पङ्क्तिं_लिख("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            पङ्क्तिं_लिख("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            पङ्क्तिं_लिख("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            यदि (v3Global.opt.तन्तवः() > 1) {
                पङ्क्तिं_लिख("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                पङ्क्तिं_लिख("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
                पङ्क्तिं_लिख("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            पङ्क्तिं_लिख("static uint32_t fake_zero_count = 0;\n");
            पङ्क्तिं_लिख("std::string fullhier = std::string{vlNamep} + hierp;\n");
            पङ्क्तिं_लिख("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            पङ्क्तिं_लिख("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            पङ्क्तिं_लिख("*count32p = 0;\n");
            पङ्क्तिं_लिख("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            पङ्क्तिं_लिख("  \"filename\",filenamep,");
            पङ्क्तिं_लिख("  \"lineno\",lineno,");
            पङ्क्तिं_लिख("  \"column\",column,\n");
            पङ्क्तिं_लिख("\"hier\",fullhier.c_str(),");
            पङ्क्तिं_लिख("  \"page\",pagep,");
            पङ्क्तिं_लिख("  \"comment\",commentp,");
            पङ्क्तिं_लिख("  (linescovp[0] ? \"linescov\" : \"\"), linescovp,");
            पङ्क्तिं_लिख("  (fsmVarp[0] ? \"fsm_var\" : \"\"), fsmVarp,");
            पङ्क्तिं_लिख("  (fsmFromp[0] ? \"fsm_from\" : \"\"), fsmFromp,");
            पङ्क्तिं_लिख("  (fsmTop[0] ? \"fsm_to\" : \"\"), fsmTop,");
            पङ्क्तिं_लिख("  (fsmTagp[0] ? \"fsm_tag\" : \"\"), fsmTagp);\n");
            पङ्क्तिं_लिख("}\n");
        }
        यदि (v3Global.opt.coverageToggle()) {
            पङ्क्तिं_लिख("\n// Toggle Coverage\n");
            पङ्क्तिं_लिख("void " + EmitCUtil::उपसर्गः_Name_Protect(m_modp) + "::__vlCoverToggleInsert(");
            पङ्क्तिं_लिख("int begin, int end, bool ranged, ");
            पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            पङ्क्तिं_लिख("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            पङ्क्तिं_लिख("const char* hierp, const char* pagep, const char* commentp) {\n");
            यदि (v3Global.opt.तन्तवः() > 1) {
                पङ्क्तिं_लिख("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            पङ्क्तिं_लिख("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            पङ्क्तिं_लिख("for (int i = begin; i != end + step; i += step) {\n");
            पङ्क्तिं_लिख("for (int j = 0; j < 2; j++) {\n");
            यदि (v3Global.opt.तन्तवः() > 1) {
                पङ्क्तिं_लिख("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
                पङ्क्तिं_लिख("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            पङ्क्तिं_लिख("static uint32_t fake_zero_count = 0;\n");
            पङ्क्तिं_लिख("std::string fullhier = std::string{vlNamep} + hierp;\n");
            पङ्क्तिं_लिख("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            पङ्क्तिं_लिख("std::string commentWithIndex = commentp;\n");
            पङ्क्तिं_लिख("if (ranged) commentWithIndex += '[' + std::to_string(i) + ']';\n");
            पङ्क्तिं_लिख("commentWithIndex += j ? \":0->1\" : \":1->0\";\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            पङ्क्तिं_लिख("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            पङ्क्तिं_लिख("*count32p = 0;\n");
            पङ्क्तिं_लिख("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            पङ्क्तिं_लिख("  \"filename\",filenamep,");
            पङ्क्तिं_लिख("  \"lineno\",lineno,");
            पङ्क्तिं_लिख("  \"column\",column,\n");
            पङ्क्तिं_लिख("\"hier\",fullhier.c_str(),");
            पङ्क्तिं_लिख("  \"page\",pagep,");
            पङ्क्तिं_लिख("  \"comment\",commentWithIndex.c_str(),");
            पङ्क्तिं_लिख("  \"\", \"\");\n");  //  linescov argument, but in toggle coverage it is always
                                       //रिक्तानि
            पङ्क्तिं_लिख("++countp;\n");
            पङ्क्तिं_लिख("}\n");
            पङ्क्तिं_लिख("}\n");
            पङ्क्तिं_लिख("}\n");
        }
    }
    शून्य emitSavableImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        यदि (v3Global.opt.savable()) {
            पङ्क्तिं_लिख("\n// Savable\n");
            प्रति (पूर्णाङ्क de = 0; de < 2; ++de) {
                ध्रुव string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ध्रुव string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ध्रुव string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::उपसर्गः_Name_Protect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        hash.निवेशय(varp->name());
                        hash.निवेशय(varp->dtypep()->विस्तारः());
                    }
                }
                ofp()->प्रारूप_लिख("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                यदि (de) {
                    पङ्क्तिं_लिख("os.readAssert(__Vcheckval);\n");
                } अन्यथा {
                    पङ्क्तिं_लिख("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                पङ्क्तिं_लिख("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        यदि (varp->isIO() && modp->is_शिखरम्() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } अन्यथा यदि (varp->isParam()) {
                        } अन्यथा यदि (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } अन्यथा {
                            पूर्णाङ्क vects = 0;
                            Ast_ग्रन्थिः_DType* elementp = varp->dtypeSkipRefp();
                            प्रति (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ध्रुव पूर्णाङ्क vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ध्रुव string ivar = "__Vi"s + cvt_गन्तव्यम्_Str(vecnum);
                                पङ्क्तिं_लिख("for (int __Vi" + cvt_गन्तव्यम्_Str(vecnum) + " = " + cvt_गन्तव्यम्_Str(0));
                                पङ्क्तिं_लिख("; " + ivar + " < " + cvt_गन्तव्यम्_Str(arrayp->elementsConst()));
                                पङ्क्तिं_लिख("; ++" + ivar + ") {\n");
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
                                ध्रुव string ivar = "__Vi"s + cvt_गन्तव्यम्_Str(vecnum);
                                पङ्क्तिं_लिख("for (int __Vi" + cvt_गन्तव्यम्_Str(vecnum) + " = " + cvt_गन्तव्यम्_Str(0));
                                पङ्क्तिं_लिख("; " + ivar + " < " + cvt_गन्तव्यम्_Str(elementp->विस्तारः_शब्दाः()));
                                पङ्क्तिं_लिख("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            प्रति (पूर्णाङ्क v = 0; v < vects; ++v) पङ्क्तिं_लिख("[__Vi" + cvt_गन्तव्यम्_Str(v) + "]");
                            पङ्क्तिं_लिख(";\n");
                            प्रति (पूर्णाङ्क v = 0; v < vects; ++v) पङ्क्तिं_लिख("}\n");
                        }
                    }
                }

                पङ्क्तिं_लिख("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    तार्किक hasCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) ध्रुव {
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
    शून्य doCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
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
    शून्य emitCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव AstClass* ध्रुव classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : शून्यसूचकः;

        यदि (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            यदि (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_अग्र_Output_File(m_file_आधारः_Name);

            doCommonImp(modp);
            यदि (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    शून्य emitCFuncImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Functions to be emitted here
        std::सदिशम्<AstCFunc*> funcps;

        ध्रुव स्वचालित gather = [एतत्, &funcps](ध्रुव Ast_ग्रन्थिः_Module* modp) {
            प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
                यदि (!funcp) अनुवर्त;
                // TRACE_* and DPI handled elsewhere
                यदि (funcp->isTrace()) अनुवर्त;
                यदि (funcp->dpiImportPrototype()) अनुवर्त;
                यदि (funcp->dpiExportDispatcher()) अनुवर्त;
                यदि (funcp->slow() != m_slow) अनुवर्त;
                funcps.प्रक्षिप_पृष्ठभागः(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        यदि (funcps.रिक्तानि()) प्रत्यागम;

        // Open output file
        open_अग्र_Output_File(m_unique_नामानि.get(m_file_आधारः_Name));
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
    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (विभज_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सत्य);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_अग्र_Output_File(m_unique_नामानि.get(m_file_आधारः_Name));
        }

        EmitCFunc::दर्शय(nodep);
    }

    स्पष्ट EmitCImp(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::उपसर्गः_Name_Protect(modp));

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
    स्थिर std::सदिशम्<AstCFile*> मुख्यम्(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक slow) VL_MT_STABLE {
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
    पूर्णाङ्क m_enum_अङ्कः = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ग्रन्थिः*, पूर्णाङ्क> m_enum_अङ्कः_Map;  // EnumDType to enumeration number
    पूर्णाङ्क m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3_Unique_नामानि m_unique_नामानि;  // Generates unique file names
    ध्रुव std::string m_file_आधारः_Name = EmitCUtil::शिखरम्_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ध्रुव आकार_t m_विभज_सीमा = v3Global.opt.output_विभज_CTrace()
                                    ? static_cast<आकार_t>(v3Global.opt.output_विभज_CTrace())
                                    : std::numeric_limits<आकार_t>::max();

    शून्य open_अग्र_Output_File() {
        openNewOutputSourceFile(m_unique_नामानि.get(m_file_आधारः_Name), सत्य, सत्य,
                                "Tracing declarations");
        पङ्क्तिं_लिख("\n");
        प्रति (ध्रुव std::string& आधारः : v3Global.opt.traceSourceLangs()) {
            पङ्क्तिं_लिख("#include \"" + आधारः + ".h\"\n");
        }
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("\nvoid " + EmitCUtil::उपसर्गः_Name_Protect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::गन्तव्यम्_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.trace_Class_आधारः() + "* tracep) {\n");
    }

सार्वजनिक:
    // METHODS
    पूर्णाङ्क get_Enum_Map_अङ्कः(AstEnumDType* nodep) {
        पूर्णाङ्क& enumNumr = m_enum_अङ्कः_Map[nodep];
        यदि (!enumNumr) {
            यदि (विभज_Needed(m_विभज_सीमा)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                पङ्क्तिं_लिख("}\n");
                closeOutputFile();
                open_अग्र_Output_File();
            }

            enumNumr = ++m_enum_अङ्कः;
            पूर्णाङ्क nvals = 0;
            पङ्क्तिं_लिख("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            पङ्क्तिं_लिख("= {");
            प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                यदि (++nvals > 1) पङ्क्तिं_लिख(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            पङ्क्तिं_लिख("};\n");
            nvals = 0;
            पङ्क्तिं_लिख("const char* " + protect("__VenumItemValues") + "[]\n");
            पङ्क्तिं_लिख("= {");
            प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                यदि (++nvals > 1) पङ्क्तिं_लिख(", ");
                putbs("\"" + constp->अङ्कः().displayed(nodep, "%0b") + "\"");
            }
            पङ्क्तिं_लिख("};\n");
            पङ्क्तिं_लिख("tracep->declDTypeEnum(" + std::गन्तव्यम्_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::गन्तव्यम्_string(nvals) + ", " + std::गन्तव्यम्_string(nodep->विस्तारः_Min()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            पङ्क्तिं_लिख("}\n");
            विभज_आकार_Inc(Ast_ग्रन्थिः::INSTR_गणनम्_CALL);
        }
        प्रत्यागम enumNumr;
    }

    // Close output file
    शून्य finalize() {
        // Close function definition
        पङ्क्तिं_लिख("}\n");

        ध्रुव std::string modName = EmitCUtil::उपसर्गः_Name_Protect(m_modp);
        ध्रुव std::string तर्काः = v3Global.opt.trace_Class_आधारः() + "* tracep";

        // Forward declarations for subs in other files
        प्रति (पूर्णाङ्क i = 0; i < m_traceTypeSubs - 1; ++i) {
            पङ्क्तिं_लिख("void " + modName + "__" + protect("traceDeclTypesSub" + std::गन्तव्यम्_string(i)) + "("
                 + तर्काः + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        पङ्क्तिं_लिख("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + तर्काः + ") {\n");
        प्रति (पूर्णाङ्क i = 0; i < m_traceTypeSubs; ++i) {
            पङ्क्तिं_लिख(modName + "__" + protect("traceDeclTypesSub" + std::गन्तव्यम्_string(i))
                 + "(tracep);\n");
        }
        पङ्क्तिं_लिख("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->शिखरम्_Modulep();
        open_अग्र_Output_File();
    }
    ~EmitCTraceTypes() override = स्वभाव;
};

वर्ग EmitCTrace final : सार्वजनिक EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ध्रुव तार्किक m_slow;  // Making slow file
    ध्रुव std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? नूतन EmitCTraceTypes{} : शून्यसूचकः};
    V3_Unique_नामानि m_unique_नामानि;  // Generates unique file names
    ध्रुव std::string m_file_आधारः_Name = EmitCUtil::शिखरम्_Class_Name() + "_" + protect("_Trace");

    // METHODS
    शून्य open_अग्र_Output_File() {
        openNewOutputSourceFile(m_unique_नामानि.get(m_file_आधारः_Name), m_slow, सत्य,
                                "Tracing implementation internals");
        पङ्क्तिं_लिख("\n");
        प्रति (ध्रुव std::string& आधारः : v3Global.opt.traceSourceLangs()) {
            पङ्क्तिं_लिख("#include \"" + आधारः + ".h\"\n");
        }
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        पङ्क्तिं_लिख("\n");
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

    शून्य emitTraceInitOne(ध्रुव AstTraceDecl* nodep, पूर्णाङ्क enum_अङ्कः) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ध्रुव callp = nodep->dtypeCallp();
        यदि (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvt_गन्तव्यम्_Str(nodep->fidx()) + ", c+"
                            + cvt_गन्तव्यम्_Str(nodep->संहिता()) + ", " + direction);
            प्रत्यागम;
        }

        यदि (nodep->dtypep()->basicp()->isDouble()) {
            पङ्क्तिं_लिख("VL_TRACE_DECL_DOUBLE");
        } अन्यथा यदि (nodep->isWide()) {
            पङ्क्तिं_लिख("VL_TRACE_DECL_WIDE");
        } अन्यथा यदि (nodep->isQuad()) {
            पङ्क्तिं_लिख("VL_TRACE_DECL_QUAD");
        } अन्यथा यदि (nodep->bitRange().ranged()) {
            पङ्क्तिं_लिख("VL_TRACE_DECL_BUS");
        } अन्यथा यदि (nodep->dtypep()->basicp()->isEvent()) {
            पङ्क्तिं_लिख("VL_TRACE_DECL_EVENT");
        } अन्यथा {
            पङ्क्तिं_लिख("VL_TRACE_DECL_BIT");
        }

        यदि (nodep->arrayRange().ranged()) {
            पङ्क्तिं_लिख("_ARRAY(tracep");
        } अन्यथा {
            पङ्क्तिं_लिख("(tracep");
        }

        //संहिता
        पङ्क्तिं_लिख(",c+" + cvt_गन्तव्यम्_Str(nodep->संहिता()));
        यदि (nodep->arrayRange().ranged()) पङ्क्तिं_लिख("+i*" + cvt_गन्तव्यम्_Str(nodep->विस्तारः_शब्दाः()));

        // Function index
        पङ्क्तिं_लिख(",");
        यदि (nodep->अन्तर्_Dtype_Func()) {
            पङ्क्तिं_लिख("fidx");
        } अन्यथा {
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->fidx()));
        }

        // Name
        पङ्क्तिं_लिख(",");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->showname(), nodep->protect()));

        // Enum number
        पङ्क्तिं_लिख("," + cvt_गन्तव्यम्_Str(enum_अङ्कः));

        // Direction
        यदि (nodep->अन्तर्_Dtype_Func()) {
            पङ्क्तिं_लिख(", direction");
        } अन्यथा {
            पङ्क्तिं_लिख(", " + direction);
        }

        //विधा
        पङ्क्तिं_लिख(", VerilatedTraceSigKind::");
        पङ्क्तिं_लिख(nodep->varType().trace_Sig_विधा());

        // Type
        पङ्क्तिं_लिख(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        पङ्क्तिं_लिख(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        यदि (nodep->arrayRange().ranged()) {
            यदि (nodep->arrayRange().ascending()) {
                पङ्क्तिं_लिख(", (i + " + std::गन्तव्यम्_string(nodep->arrayRange().lo()) + ")");
            } अन्यथा {
                पङ्क्तिं_लिख(", (" + std::गन्तव्यम्_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        यदि (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            पङ्क्तिं_लिख(", " + cvt_गन्तव्यम्_Str(nodep->bitRange().वामतः()) + ","
                 + cvt_गन्तव्यम्_Str(nodep->bitRange().दक्षिणतः()));
        }

        //
        पङ्क्तिं_लिख(");");
    }

    पूर्णाङ्क emitTraceDeclDType(Ast_ग्रन्थिः_DType* nodep) {
        // Return enum number or -1 for none
        यदि (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            यदि (AstEnumDType* ध्रुव enump = VN_CAST(nodep->skip_Ref_गन्तव्यम्_Enump(), EnumDType)) {
                प्रत्यागम m_emitTypesp->get_Enum_Map_अङ्कः(enump);
            }
        }
        प्रत्यागम -1;
    }

    शून्य emitTraceChangeOne(AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ध्रुव std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        तार्किक emit_विस्तारः = सत्य;
        string stype;
        यदि (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emit_विस्तारः = असत्य;
        } अन्यथा यदि (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } अन्यथा यदि (nodep->isQuad()) {
            stype = "QData";
        } अन्यथा यदि (nodep->declp()->विस्तारः_Min() > 16) {
            stype = "IData";
        } अन्यथा यदि (nodep->declp()->विस्तारः_Min() > 8) {
            stype = "SData";
        } अन्यथा यदि (nodep->declp()->विस्तारः_Min() > 1) {
            stype = "CData";
        } अन्यथा यदि (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emit_विस्तारः = असत्य;
        } अन्यथा {
            stype = "Bit";
            emit_विस्तारः = असत्य;
        }
        putns(nodep, "bufp->" + func + stype);

        ध्रुव uint32_t विस्थापनम् = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->विस्तारः_शब्दाः());
        ध्रुव uint32_t संहिता = nodep->declp()->संहिता() + विस्थापनम्;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        पङ्क्तिं_लिख("(oldp+");
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(संहिता - nodep->आधारः_संहिता()));
        पङ्क्तिं_लिख(",");
        ध्रुव VNumRange& arrayRange = nodep->declp()->arrayRange();
        यदि (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_मानम्(nodep, arrayindex);
        यदि (emit_विस्तारः) पङ्क्तिं_लिख("," + cvt_गन्तव्यम्_Str(nodep->declp()->विस्तारः_Min()));
        पङ्क्तिं_लिख(");\n");
    }

    शून्य emit_Trace_मानम्(ध्रुव AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ध्रुव AstVar* ध्रुव varp = varrefp->varp();
            यदि (varp->isEvent()) पङ्क्तिं_लिख("&");
            पङ्क्तिं_लिख("(");
            यदि (emitTraceIsScBigUint(nodep)) {
                पङ्क्तिं_लिख("(uint32_t*)");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                पङ्क्तिं_लिख("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_सूचकांक(nodep, arrayindex);
            यदि (varp->isSc()) पङ्क्तिं_लिख(".read()");
            यदि (emitTraceIsScUint(nodep)) {
                पङ्क्तिं_लिख(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } अन्यथा यदि (emitTraceIsScBigUint(nodep)) {
                पङ्क्तिं_लिख(".get_raw()");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                पङ्क्तिं_लिख(")");
            }
            पङ्क्तिं_लिख(")");
        } अन्यथा {
            पङ्क्तिं_लिख("(");
            iterateConst(nodep->valuep());
            emit_Trace_सूचकांक(nodep, arrayindex);
            पङ्क्तिं_लिख(")");
        }
    }

    शून्य emit_Trace_सूचकांक(ध्रुव AstTraceInc* ध्रुव nodep, पूर्णाङ्क arrayindex) {
        // Tracing only supports 1D arrays
        यदि (nodep->declp()->arrayRange().ranged()) {
            यदि (arrayindex == -2) {
                पङ्क्तिं_लिख("[i]");
            } अन्यथा यदि (arrayindex == -1) {
                पङ्क्तिं_लिख("[0]");
            } अन्यथा {
                पङ्क्तिं_लिख("[" + cvt_गन्तव्यम्_Str(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    उपयुज्य EmitCFunc::दर्शय;  // Suppress hidden overloaded virtual function warning
    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (!nodep->isTrace()) प्रत्यागम;
        यदि (nodep->slow() != m_slow) प्रत्यागम;

        यदि (विभज_Needed()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सत्य);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_अग्र_Output_File();
        }

        EmitCFunc::दर्शय(nodep);
    }
    शून्य दर्शय(Ast_Trace_प्रक्षिप_उपसर्गः* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        यदि (nodep->quoted_उपसर्गः()) {
            putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->उपसर्गः(), nodep->protect()));
        } अन्यथा {
            पङ्क्तिं_लिख(nodep->उपसर्गः());
        }
        पङ्क्तिं_लिख(", VerilatedTracePrefixType::");
        पङ्क्तिं_लिख(nodep->उपसर्गः_Type().ascii());
        पङ्क्तिं_लिख(", " + std::गन्तव्यम्_string(nodep->वामतः()));
        पङ्क्तिं_लिख(", " + std::गन्तव्यम्_string(nodep->दक्षिणतः()));
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(Ast_Trace_निष्कासय_उपसर्गः* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    शून्य दर्शय(AstTraceDecl* nodep) override {
        ध्रुव पूर्णाङ्क enum_अङ्कः = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        यदि (nodep->arrayRange().ranged()) {
            पङ्क्तिं_लिख("for (int i = 0; i < " + cvt_गन्तव्यम्_Str(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_अङ्कः);
            पङ्क्तिं_लिख("\n}\n");
        } अन्यथा {
            emitTraceInitOne(nodep, enum_अङ्कः);
            पङ्क्तिं_लिख("\n");
        }
    }
    शून्य दर्शय(AstTraceInc* nodep) override {
        यदि (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            प्रति (पूर्णाङ्क i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } अन्यथा {
            emitTraceChangeOne(nodep, -1);
        }
    }

    स्पष्ट EmitCTrace(तार्किक slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->शिखरम्_Modulep();
        // Open output file
        open_अग्र_Output_File();
        // Emit functions
        प्रति (Ast_ग्रन्थिः* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        यदि (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = स्वभाव;

सार्वजनिक:
    स्थिर std::सदिशम्<AstCFile*> मुख्यम्(तार्किक slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::सदिशम्<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        यदि (slow) {
            प्रति (AstCFile* ध्रुव cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_पृष्ठभागः(cfilep);
            }
        }
        प्रत्यागम cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

वर्ग EmitCFile final : सार्वजनिक EmitCFunc {
    स्पष्ट EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य मुख्यम्(AstCFile* cfilep) VL_MT_STABLE {
        यदि (!cfilep->tblockp()) प्रत्यागम;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::सदिशम्<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ध्रुव EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            यदि (VN_IS(nodep, Class)) अनुवर्त;  // Imped with ClassPackage
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(nodep, ग्रन्थिः_Module);
            यदि (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
            cfiles.emplace_पृष्ठभागः();
            std::सदिशम्<AstCFile*>& slow = cfiles.पृष्ठभागः();
            threadScope.पङ्क्तौ_निक्षिप([modp, &slow] { slow = EmitCImp::मुख्यम्(modp, /* slow: */ सत्य); });
            cfiles.emplace_पृष्ठभागः();
            std::सदिशम्<AstCFile*>& fast = cfiles.पृष्ठभागः();
            threadScope.पङ्क्तौ_निक्षिप([modp, &fast] { fast = EmitCImp::मुख्यम्(modp, /* slow: */ असत्य); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        यदि (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_पृष्ठभागः();
            std::सदिशम्<AstCFile*>& slow = cfiles.पृष्ठभागः();
            threadScope.पङ्क्तौ_निक्षिप([&slow] { slow = EmitCTrace::मुख्यम्(/* slow: */ सत्य); });
            cfiles.emplace_पृष्ठभागः();
            std::सदिशम्<AstCFile*>& fast = cfiles.पृष्ठभागः();
            threadScope.पङ्क्तौ_निक्षिप([&fast] { fast = EmitCTrace::मुख्यम्(/* slow: */ असत्य); });
        }
    }
    // Add files to netlist
    प्रति (ध्रुव std::सदिशम्<AstCFile*>& cfileps : cfiles) {
        प्रति (AstCFile* ध्रुव cfilep : cfileps) v3Global.rootp()->सङ्कलय_Filesp(cfilep);
    }
}

शून्य V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    प्रति (Ast_ग्रन्थिः_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ग्रन्थिः_File);
        यदि (AstCFile* ध्रुव cfilep = VN_CAST(filep, CFile)) EmitCFile::मुख्यम्(cfilep);
    }
}
