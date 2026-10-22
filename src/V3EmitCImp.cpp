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
        สำหรับ (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ถ้า (const AstVar* const varp = VN_CAST(nodep, Var)) {
                ถ้า (varp->isParam()) {
                    ถ้า (first) {
                        puts("\n");
                        const string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = false;
                    }
                    ถ้า (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        ถ้า (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    static void addVarCtorDtorStmts(TextStmts& stmts, const AstVar* varp, bool destroy) {
        const std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        ถ้า (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            สำหรับ (const AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                สำหรับ (const std::string& elem : elems) {
                    สำหรับ (จำนวนเต็ม i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.push_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            ถ้า (destroy) std::reverse(elems.begin(), elems.end());
            สำหรับ (const std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            คืน;
        }

        // Other variables are constructed whole, even if aggregates
        ถ้า (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            คืน;
        }
        const AstBasicDType* const basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        ถ้า (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } มิฉะนั้น ถ้า (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->num().emitC();
        } มิฉะนั้น ถ้า (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    void createCtorDtorStmts(const AstNodeModule* modp) {
        // Construct member variables
        สำหรับ (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            ถ้า (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, false);
            }
        }

        // Destroy in reverse order
        const AstNode* const lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        สำหรับ (const AstNode* nodep = lastp; nodep; nodep = nodep->prevp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            ถ้า (varp && isExplicitlyConstructed(varp)) {
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
        ถ้า (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        ถ้า (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        ถ้า (modp->isTop()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } มิฉะนั้น {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        สำหรับ (const TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        ถ้า (modp->ctorVarReset()) {
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
        ถ้า (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        สำหรับ (const TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
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

        ถ้า (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        ถ้า (v3Global.opt.coverage()) {
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
        ถ้า (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            ถ้า (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } มิฉะนั้น {
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
        ถ้า (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            ถ้า (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            ถ้า (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } มิฉะนั้น {
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
        ถ้า (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            สำหรับ (จำนวนเต็ม de = 0; de < 2; ++de) {
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
                สำหรับ (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ถ้า (const AstVar* const varp = VN_CAST(nodep, Var)) {
                        hash.insert(varp->name());
                        hash.insert(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                ถ้า (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } มิฉะนั้น {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                สำหรับ (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    ถ้า (const AstVar* const varp = VN_CAST(nodep, Var)) {
                        ถ้า (varp->isIO() && modp->isTop() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } มิฉะนั้น ถ้า (varp->isParam()) {
                        } มิฉะนั้น ถ้า (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } มิฉะนั้น {
                            จำนวนเต็ม vects = 0;
                            AstNodeDType* elementp = varp->dtypeSkipRefp();
                            สำหรับ (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                const จำนวนเต็ม vecnum = vects++;
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
                            ถ้า (basicp && basicp->keyword().isMTaskState()) continue;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            ถ้า (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                const จำนวนเต็ม vecnum = vects++;
                                const string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->widthWords()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            สำหรับ (จำนวนเต็ม v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            สำหรับ (จำนวนเต็ม v = 0; v < vects; ++v) puts("}\n");
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
        ถ้า (!modp) คืน false;
        // We always need the slow file
        ถ้า (m_slow) คืน true;
        // The fast file is only required when we have `systemc_implementation nodes
        สำหรับ (const AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            ถ้า (scSectionp->sectionType() == VSystemCSectionType::IMP) คืน true;
        }
        คืน false;
    }
    // Actually emit common implementation contents for given AstNodeModule
    void doCommonImp(const AstNodeModule* modp) {
        ถ้า (m_slow) {
            ถ้า (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } มิฉะนั้น {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    void emitCommonImp(const AstNodeModule* modp) {
        const AstClass* const classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        ถ้า (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            ถ้า (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            ถ้า (classp) {
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
            สำหรับ (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* const funcp = VN_CAST(nodep, CFunc);
                ถ้า (!funcp) continue;
                // TRACE_* and DPI handled elsewhere
                ถ้า (funcp->isTrace()) continue;
                ถ้า (funcp->dpiImportPrototype()) continue;
                ถ้า (funcp->dpiExportDispatcher()) continue;
                ถ้า (funcp->slow() != m_slow) continue;
                funcps.push_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        ถ้า (const AstClassPackage* const packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        ถ้า (funcps.empty()) คืน;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        สำหรับ (AstCFunc* const funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    void visit(AstCFunc* nodep) override {
        ถ้า (splitNeeded()) {
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
    static std::vector<AstCFile*> หลัก(const AstNodeModule* modp, bool slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        คืน emitCImp.getAndClearCfileps();
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
    จำนวนเต็ม m_enumNum = 0;  // Enumeration number (whole netlist)
    std::unordered_map<AstNode*, จำนวนเต็ม> m_enumNumMap;  // EnumDType to enumeration number
    จำนวนเต็ม m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
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
        สำหรับ (const std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

public:
    // METHODS
    จำนวนเต็ม getEnumMapNum(AstEnumDType* nodep) {
        จำนวนเต็ม& enumNumr = m_enumNumMap[nodep];
        ถ้า (!enumNumr) {
            ถ้า (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enumNum;
            จำนวนเต็ม nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            สำหรับ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ถ้า (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            สำหรับ (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* const constp = VN_AS(itemp->valuep(), Const);
                ถ้า (++nvals > 1) puts(", ");
                putbs("\"" + constp->num().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(AstNode::INSTR_COUNT_CALL);
        }
        คืน enumNumr;
    }

    // Close output file
    void finalize() {
        // Close function definition
        puts("}\n");

        const std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        const std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        สำหรับ (จำนวนเต็ม i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        สำหรับ (จำนวนเต็ม i = 0; i < m_traceTypeSubs; ++i) {
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
        สำหรับ (const std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    bool emitTraceIsScBv(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ถ้า (!varrefp) คืน false;
        const AstVar* const varp = varrefp->varp();
        คืน varp->isSc() && varp->isScBv();
    }

    bool emitTraceIsScBigUint(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ถ้า (!varrefp) คืน false;
        const AstVar* const varp = varrefp->varp();
        คืน varp->isSc() && varp->isScBigUint();
    }

    bool emitTraceIsScUint(const AstTraceInc* nodep) {
        const AstVarRef* const varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        ถ้า (!varrefp) คืน false;
        const AstVar* const varp = varrefp->varp();
        คืน varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    void emitTraceInitOne(const AstTraceDecl* nodep, จำนวนเต็ม enumNum) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* const callp = nodep->dtypeCallp();
        ถ้า (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            คืน;
        }

        ถ้า (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } มิฉะนั้น ถ้า (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } มิฉะนั้น ถ้า (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } มิฉะนั้น ถ้า (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } มิฉะนั้น ถ้า (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } มิฉะนั้น {
            puts("VL_TRACE_DECL_BIT");
        }

        ถ้า (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } มิฉะนั้น {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        ถ้า (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->widthWords()));

        // Function index
        puts(",");
        ถ้า (nodep->inDtypeFunc()) {
            puts("fidx");
        } มิฉะนั้น {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protectWordsIf(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enumNum));

        // Direction
        ถ้า (nodep->inDtypeFunc()) {
            puts(", direction");
        } มิฉะนั้น {
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
        ถ้า (nodep->arrayRange().ranged()) {
            ถ้า (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } มิฉะนั้น {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        ถ้า (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().left()) + ","
                 + cvtToStr(nodep->bitRange().right()));
        }

        //
        puts(");");
    }

    จำนวนเต็ม emitTraceDeclDType(AstNodeDType* nodep) {
        // Return enum number or -1 for none
        ถ้า (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            ถ้า (AstEnumDType* const enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                คืน m_emitTypesp->getEnumMapNum(enump);
            }
        }
        คืน -1;
    }

    void emitTraceChangeOne(AstTraceInc* nodep, จำนวนเต็ม arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        const std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        bool emitWidth = true;
        string stype;
        ถ้า (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = false;
        } มิฉะนั้น ถ้า (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } มิฉะนั้น ถ้า (nodep->isQuad()) {
            stype = "QData";
        } มิฉะนั้น ถ้า (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } มิฉะนั้น ถ้า (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } มิฉะนั้น ถ้า (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } มิฉะนั้น ถ้า (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = false;
        } มิฉะนั้น {
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
        ถ้า (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emitTraceValue(nodep, arrayindex);
        ถ้า (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    void emitTraceValue(const AstTraceInc* nodep, จำนวนเต็ม arrayindex) {
        ถ้า (AstVarRef* const varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            const AstVar* const varp = varrefp->varp();
            ถ้า (varp->isEvent()) puts("&");
            puts("(");
            ถ้า (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } มิฉะนั้น ถ้า (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            ถ้า (varp->isSc()) puts(".read()");
            ถ้า (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } มิฉะนั้น ถ้า (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } มิฉะนั้น ถ้า (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } มิฉะนั้น {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    void emitTraceIndex(const AstTraceInc* const nodep, จำนวนเต็ม arrayindex) {
        // Tracing only supports 1D arrays
        ถ้า (nodep->declp()->arrayRange().ranged()) {
            ถ้า (arrayindex == -2) {
                puts("[i]");
            } มิฉะนั้น ถ้า (arrayindex == -1) {
                puts("[0]");
            } มิฉะนั้น {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    void visit(AstCFunc* nodep) override {
        ถ้า (!nodep->isTrace()) คืน;
        ถ้า (nodep->slow() != m_slow) คืน;

        ถ้า (splitNeeded()) {
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
        ถ้า (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protectWordsIf(nodep->prefix(), nodep->protect()));
        } มิฉะนั้น {
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
        const จำนวนเต็ม enumNum = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        ถ้า (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enumNum);
            puts("\n}\n");
        } มิฉะนั้น {
            emitTraceInitOne(nodep, enumNum);
            puts("\n");
        }
    }
    void visit(AstTraceInc* nodep) override {
        ถ้า (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            สำหรับ (จำนวนเต็ม i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } มิฉะนั้น {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(bool slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->topModulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        สำหรับ (AstNode* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ถ้า (AstCFunc* const funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        ถ้า (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = default;

public:
    static std::vector<AstCFile*> หลัก(bool slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        ถ้า (slow) {
            สำหรับ (AstCFile* const cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        คืน cfileps;
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
    static void หลัก(AstCFile* cfilep) VL_MT_STABLE {
        ถ้า (!cfilep->tblockp()) คืน;
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
        สำหรับ (const AstNode* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            ถ้า (VN_IS(nodep, Class)) continue;  // Imped with ClassPackage
            const AstNodeModule* const modp = VN_AS(nodep, NodeModule);
            ถ้า (modp->isConstPool()) continue;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::หลัก(modp, /* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::หลัก(modp, /* slow: */ false); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        ถ้า (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::หลัก(/* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::หลัก(/* slow: */ false); });
        }
    }
    // Add files to netlist
    สำหรับ (const std::vector<AstCFile*>& cfileps : cfiles) {
        สำหรับ (AstCFile* const cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

void V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    สำหรับ (AstNodeFile *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), NodeFile);
        ถ้า (AstCFile* const cfilep = VN_CAST(filep, CFile)) EmitCFile::หลัก(cfilep);
    }
}
