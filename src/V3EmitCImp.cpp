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

#include "V3PchAstMT.h"

#include "V3EmitC.h"
#include "V3EmitCFunc.h"
#include "V3ThreadPool.h"
#include "V3UniqueNames.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

class EmitCImp final : public EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    const AstNodeModule* const m_fileModp;
    const bool m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    const std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    void openNextOutputFile(const std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, false, "Design implementation internals");
        emitFileHeader();
    }
    void emitFileHeader() {
        puts("// See " + EmitCUtil::topClassName() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    void emitParamDefns(const AstNodeModule* modp) {
        bool first = true;
        for (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ᱡᱩᱫᱤ (const AstVar* const varp = VN_CAST(nodep, Var)) {
                ᱡᱩᱫᱤ (varp->isParam()) {
                    ᱡᱩᱫᱤ (first) {
                        puts("\n");
                        const string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = false;
                    }
                    ᱡᱩᱫᱤ (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ᱡᱩᱫᱤ (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    static void addVarCtorDtorStmts(TextStmts& stmts, const AstVar* varp, bool destroy) {
        const std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ᱡᱩᱫᱤ (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            for (const AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                for (const std::string& elem : elems) {
                    for (puur_naa_nka i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.push_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ᱡᱩᱫᱤ (destroy) std::reverse(elems.begin(), elems.end());
            for (const std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            ᱨᱩᱣᱟᱹᱲ;
        }

        // Other variables are constructed whole, even if aggregates
        ᱡᱩᱫᱤ (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            ᱨᱩᱣᱟᱹᱲ;
        }
        const AstBasicDType* const basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        ᱡᱩᱫᱤ (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->num().emitC();
        } ᱵᱟᱝ ᱡᱩᱫᱤ (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    void createCtorDtorStmts(const AstNodeModule* modp) {
        // Construct member variables
        for (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            ᱡᱩᱫᱤ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, false);
            }
        }

        // Destroy in reverse order
        const AstNode* const lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        for (const AstNode* nodep = lastp; nodep; nodep = nodep->prevp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            ᱡᱩᱫᱤ (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, true);
            }
        }

        // Split into sub-functions if needed
        const std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", false,
            "Design implementation internals", [this]() { emitFileHeader(); });
    }
    void emitCtorDtorImp(const AstNodeModule* modp) {
        const std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        ᱡᱩᱫᱤ (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ᱡᱩᱫᱤ (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ᱡᱩᱫᱤ (modp->isTop()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } ᱵᱟᱝ {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        for (const TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ᱡᱩᱫᱤ (modp->ctorVarReset()) {
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
        ᱡᱩᱫᱤ (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        for (const TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    void emitConfigureImp(const AstNodeModule* modp) {
        const string modName = EmitCUtil::prefixNameProtect(modp);

        ᱡᱩᱫᱤ (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ᱡᱩᱫᱤ (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    void emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        ᱡᱩᱫᱤ (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ᱡᱩᱫᱤ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ᱵᱟᱝ {
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
        ᱡᱩᱫᱤ (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ᱡᱩᱫᱤ (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ᱡᱩᱫᱤ (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } ᱵᱟᱝ {
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
    void emitSavableImp(const AstNodeModule* modp) {
        ᱡᱩᱫᱤ (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            for (puur_naa_nka de = 0; de < 2; ++de) {
                const string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                const string funcname = de ? "__Vdeserialize" : "__Vserialize";
                const string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                for (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ᱡᱩᱫᱤ (const AstVar* const varp = VN_CAST(nodep, Var)) {
                        hash.insert(varp->name());
                        hash.insert(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ᱡᱩᱫᱤ (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } ᱵᱟᱝ {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                for (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ᱡᱩᱫᱤ (const AstVar* const varp = VN_CAST(nodep, Var)) {
                        ᱡᱩᱫᱤ (varp->isIO() && modp->isTop() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } ᱵᱟᱝ ᱡᱩᱫᱤ (varp->isParam()) {
                        } ᱵᱟᱝ ᱡᱩᱫᱤ (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } ᱵᱟᱝ {
                            puur_naa_nka vects = 0;
                            AstNodeDType* elementp = varp->dtypeSkipRefp();
                            for (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                const puur_naa_nka vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                const string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            const AstBasicDType* const basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            ᱡᱩᱫᱤ (basicp && basicp->keyword().isMTaskState()) continue;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ᱡᱩᱫᱤ (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                const puur_naa_nka vecnum = vects++;
                                const string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->widthWords()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            for (puur_naa_nka v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            for (puur_naa_nka v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    bool hasCommonImp(const AstNodeModule* modp) const {
        // Nothing to emit if no module!
        ᱡᱩᱫᱤ (!modp) ᱨᱩᱣᱟᱹᱲ false;
        // We always need the slow file
        ᱡᱩᱫᱤ (m_slow) ᱨᱩᱣᱟᱹᱲ true;
        // The fast file is only required when we have `systemc_implementation nodes
        for (const AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ᱡᱩᱫᱤ (scSectionp->sectionType() == VSystemCSectionType::IMP) ᱨᱩᱣᱟᱹᱲ true;
        }
        ᱨᱩᱣᱟᱹᱲ false;
    }
    // Actually emit common implementation contents for given AstNodeModule
    void doCommonImp(const AstNodeModule* modp) {
        ᱡᱩᱫᱤ (m_slow) {
            ᱡᱩᱫᱤ (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } ᱵᱟᱝ {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    void emitCommonImp(const AstNodeModule* modp) {
        const AstClass* const classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        ᱡᱩᱫᱤ (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ᱡᱩᱫᱤ (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            ᱡᱩᱫᱤ (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    void emitCFuncImp(const AstNodeModule* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        const auto gather = [this, &funcps](const AstNodeModule* modp) {
            for (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* const funcp = VN_CAST(nodep, CFunc);
                ᱡᱩᱫᱤ (!funcp) continue;
                // TRACE_* and DPI handled elsewhere
                ᱡᱩᱫᱤ (funcp->isTrace()) continue;
                ᱡᱩᱫᱤ (funcp->dpiImportPrototype()) continue;
                ᱡᱩᱫᱤ (funcp->dpiExportDispatcher()) continue;
                ᱡᱩᱫᱤ (funcp->slow() != m_slow) continue;
                funcps.push_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ᱡᱩᱫᱤ (const AstClassPackage* const packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ᱡᱩᱫᱤ (funcps.empty()) ᱨᱩᱣᱟᱹᱲ;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        for (AstCFunc* const funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    void visit(AstCFunc* nodep) override {
        ᱡᱩᱫᱤ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(const AstNodeModule* modp, bool slow)
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
    ~EmitCImp() override = default;

public:
    static std::vector<AstCFile*> main(const AstNodeModule* modp, bool slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        ᱨᱩᱣᱟᱹᱲ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
class EmitCTraceTypes final : public EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    puur_naa_nka m_enumNum = 0;  // Enumeration number (whole netlist)
    std::unordered_map<AstNode*, puur_naa_nka> m_enumNumMap;  // EnumDType to enumeration number
    puur_naa_nka m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    const std::string m_fileBaseName = EmitCUtil::topClassName() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    const size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    void openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), true, true,
                                "Tracing declarations");
        puts("\n");
        for (const std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

public:
    // METHODS
    puur_naa_nka getEnumMapNum(AstEnumDType* nodep) {
        puur_naa_nka& enumNumr = m_enumNumMap[nodep];
        ᱡᱩᱫᱤ (!enumNumr) {
            ᱡᱩᱫᱤ (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enumNum;
            puur_naa_nka nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            for (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ᱡᱩᱫᱤ (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            for (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* const constp = VN_AS(itemp->valuep(), Const);
                ᱡᱩᱫᱤ (++nvals > 1) puts(", ");
                putbs("\"" + constp->num().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(AstNode::INSTR_COUNT_CALL);
        }
        ᱨᱩᱣᱟᱹᱲ enumNumr;
    }

    // Close output file
    void finalize() {
        // Close function definition
        puts("}\n");

        const std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        const std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        for (puur_naa_nka i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        for (puur_naa_nka i = 0; i < m_traceTypeSubs; ++i) {
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
    ~EmitCTraceTypes() override = default;
};

class EmitCTrace final : public EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    const bool m_slow;  // Making slow file
    const std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? new EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    const std::string m_fileBaseName = EmitCUtil::topClassName() + "_" + protect("_Trace");

    // METHODS
    void openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, true,
                                "Tracing implementation internals");
        puts("\n");
        for (const std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    bool emitTraceIsScBv(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ᱡᱩᱫᱤ (!varrefp) ᱨᱩᱣᱟᱹᱲ false;
        const AstVar* const varp = varrefp->varp();
        ᱨᱩᱣᱟᱹᱲ varp->isSc() && varp->isScBv();
    }

    bool emitTraceIsScBigUint(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ᱡᱩᱫᱤ (!varrefp) ᱨᱩᱣᱟᱹᱲ false;
        const AstVar* const varp = varrefp->varp();
        ᱨᱩᱣᱟᱹᱲ varp->isSc() && varp->isScBigUint();
    }

    bool emitTraceIsScUint(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ᱡᱩᱫᱤ (!varrefp) ᱨᱩᱣᱟᱹᱲ false;
        const AstVar* const varp = varrefp->varp();
        ᱨᱩᱣᱟᱹᱲ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    void emitTraceInitOne(const AstTraceDecl* nodep, puur_naa_nka enumNum) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* const callp = nodep->dtypeCallp();
        ᱡᱩᱫᱤ (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            ᱨᱩᱣᱟᱹᱲ;
        }

        ᱡᱩᱫᱤ (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } ᱵᱟᱝ {
            puts("VL_TRACE_DECL_BIT");
        }

        ᱡᱩᱫᱤ (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } ᱵᱟᱝ {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        ᱡᱩᱫᱤ (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->widthWords()));

        // Function index
        puts(",");
        ᱡᱩᱫᱤ (nodep->inDtypeFunc()) {
            puts("fidx");
        } ᱵᱟᱝ {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protectWordsIf(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enumNum));

        // Direction
        ᱡᱩᱫᱤ (nodep->inDtypeFunc()) {
            puts(", direction");
        } ᱵᱟᱝ {
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
        ᱡᱩᱫᱤ (nodep->arrayRange().ranged()) {
            ᱡᱩᱫᱤ (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } ᱵᱟᱝ {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ᱡᱩᱫᱤ (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().left()) + ","
                 + cvtToStr(nodep->bitRange().right()));
        }

        //
        puts(");");
    }

    puur_naa_nka emitTraceDeclDType(AstNodeDType* nodep) {
        // Return enum number or -1 for none
        ᱡᱩᱫᱤ (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ᱡᱩᱫᱤ (AstEnumDType* const enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                ᱨᱩᱣᱟᱹᱲ m_emitTypesp->getEnumMapNum(enump);
            }
        }
        ᱨᱩᱣᱟᱹᱲ -1;
    }

    void emitTraceChangeOne(AstTraceInc* nodep, puur_naa_nka arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        const std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        bool emitWidth = true;
        string stype;
        ᱡᱩᱫᱤ (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = false;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->isQuad()) {
            stype = "QData";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = false;
        } ᱵᱟᱝ {
            stype = "Bit";
            emitWidth = false;
        }
        putns(nodep, "bufp->" + func + stype);

        const uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->widthWords());
        const uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        const VNumRange& arrayRange = nodep->declp()->arrayRange();
        ᱡᱩᱫᱤ (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emitTraceValue(nodep, arrayindex);
        ᱡᱩᱫᱤ (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    void emitTraceValue(const AstTraceInc* nodep, puur_naa_nka arrayindex) {
        ᱡᱩᱫᱤ (AstVarRef* const varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            const AstVar* const varp = varrefp->varp();
            ᱡᱩᱫᱤ (varp->isEvent()) puts("&");
            puts("(");
            ᱡᱩᱫᱤ (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } ᱵᱟᱝ ᱡᱩᱫᱤ (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            ᱡᱩᱫᱤ (varp->isSc()) puts(".read()");
            ᱡᱩᱫᱤ (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } ᱵᱟᱝ ᱡᱩᱫᱤ (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } ᱵᱟᱝ ᱡᱩᱫᱤ (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } ᱵᱟᱝ {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    void emitTraceIndex(const AstTraceInc* const nodep, puur_naa_nka arrayindex) {
        // Tracing only supports 1D arrays
        ᱡᱩᱫᱤ (nodep->declp()->arrayRange().ranged()) {
            ᱡᱩᱫᱤ (arrayindex == -2) {
                puts("[i]");
            } ᱵᱟᱝ ᱡᱩᱫᱤ (arrayindex == -1) {
                puts("[0]");
            } ᱵᱟᱝ {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    void visit(AstCFunc* nodep) override {
        ᱡᱩᱫᱤ (!nodep->isTrace()) ᱨᱩᱣᱟᱹᱲ;
        ᱡᱩᱫᱤ (nodep->slow() != m_slow) ᱨᱩᱣᱟᱹᱲ;

        ᱡᱩᱫᱤ (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    void visit(AstTracePushPrefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        ᱡᱩᱫᱤ (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protectWordsIf(nodep->prefix(), nodep->protect()));
        } ᱵᱟᱝ {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->left()));
        puts(", " + std::to_string(nodep->right()));
        puts(");\n");
    }
    void visit(AstTracePopPrefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    void visit(AstTraceDecl* nodep) override {
        const puur_naa_nka enumNum = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ᱡᱩᱫᱤ (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enumNum);
            puts("\n}\n");
        } ᱵᱟᱝ {
            emitTraceInitOne(nodep, enumNum);
            puts("\n");
        }
    }
    void visit(AstTraceInc* nodep) override {
        ᱡᱩᱫᱤ (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            for (puur_naa_nka i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } ᱵᱟᱝ {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(bool slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->topModulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        for (AstNode* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ᱡᱩᱫᱤ (AstCFunc* const funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ᱡᱩᱫᱤ (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = default;

public:
    static std::vector<AstCFile*> main(bool slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ᱡᱩᱫᱤ (slow) {
            for (AstCFile* const cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        ᱨᱩᱣᱟᱹᱲ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

class EmitCFile final : public EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = default;

public:
    static void main(AstCFile* cfilep) VL_MT_STABLE {
        ᱡᱩᱫᱤ (!cfilep->tblockp()) ᱨᱩᱣᱟᱹᱲ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

void V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        const EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        for (const AstNode* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ᱡᱩᱫᱤ (VN_IS(nodep, Class)) continue;  // Imped with ClassPackage
            const AstNodeModule* const modp = VN_AS(nodep, NodeModule);
            ᱡᱩᱫᱤ (modp->isConstPool()) continue;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ false); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ᱡᱩᱫᱤ (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ false); });
        }
    }
    // Add files to netlist
    for (const std::vector<AstCFile*>& cfileps : cfiles) {
        for (AstCFile* const cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

void V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    for (AstNodeFile *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), NodeFile);
        ᱡᱩᱫᱤ (AstCFile* const cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
