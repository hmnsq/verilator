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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cstdint>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ଶ୍ରେଣୀ EmitCHeader final : ସର୍ବସାଧାରଣ EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    ଶ୍ରେଣୀ Cover_ଗଣନା_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_bins = 0;  // Running total of coverage bins counted so far

        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            ଯଦି (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    ସର୍ବସାଧାରଣ:
        explicit Cover_ଗଣନା_Visitor(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
            iterateConst(const_cast<Ast_ଗଣ୍ଠି_Module*>(modp));
        }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା bins() ଅଚଳ { ଫେରନ୍ତୁ m_bins; }
    };

    ଶୂନ୍ୟ decorateFirst(ତାର୍କିକ& first, ଅଚଳ string& str) {
        ଯଦି (first) {
            putsDecoration(nullptr, str);
            first = ମିଥ୍ୟା;
        }
    }
    ଶୂନ୍ୟ emitCellDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ତାର୍କିକ first = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstCell* ଅଚଳ cellp = VN_CAST(nodep, Cell)) {
                ଯଦି (cellp->modp()->isConstPool()) ଜାରି;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    ଶୂନ୍ୟ emitDesignVarDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ଅଚଳ AstVar* portp = nullptr;
        ଅଚଳ AstVar* trivp = nullptr;
        ଅଚଳ AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
            ଯଦି (!varp) ଭାଙ୍ଗ;
            ଯଦି (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ଧାଡ଼ି_Align(), varp, "Aligned non-field");
            } ଅନ୍ୟଥା ଯଦି (varp->isPrimaryIO()) {
                ଯଦି (!portp) portp = varp;
                ++nPort;
            } ଅନ୍ୟଥା ଯଦି (varp->isTriviallyConstructible()) {
                ଯଦି (!trivp) trivp = varp;
                ++nTriv;
            } ଅନ୍ୟଥା {
                ଯଦି (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ଅଚଳ auto emitAndStep = [&](ଅଚଳ AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        ଯଦି (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            ପାଇଁ (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        ଯଦି (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            ଯଦି (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                ପାଇଁ (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } ଅନ୍ୟଥା {
                ଅଚଳ size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                ଯଦି (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } ଅନ୍ୟଥା ଯଦି (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } ଅନ୍ୟଥା ଯଦି (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                ଯଦି (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                ପାଇଁ (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    ଯଦି (anonL3s != 1) puts("struct {\n");
                    ପାଇଁ (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        ଯଦି (anonL2s != 1) puts("struct {\n");
                        ପାଇଁ (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            ଯଦି (anonL1s != 1) puts("struct {\n");
                            ପାଇଁ (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            ଯଦି (anonL1s != 1) puts("};\n");
                        }
                        ଯଦି (anonL2s != 1) puts("};\n");
                    }
                    ଯଦି (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        ଯଦି (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            ଯଦି (VN_IS(modp, Class)) {
                ପାଇଁ (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                ଫେରନ୍ତୁ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            ପାଇଁ (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    ଶୂନ୍ୟ emitInternalVarDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(modp, Class)) {
            ଯଦି (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } ଅନ୍ୟଥା {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା coverBins = Cover_ଗଣନା_Visitor{modp}.bins();
            ଯଦି (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    ଶୂନ୍ୟ emitParamDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ତାର୍କିକ first = ସତ୍ୟ;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                ଯଦି (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), ମିଥ୍ୟା, ମିଥ୍ୟା));
                    puts(";\n");
                }
            }
        }
    }
    ଶୂନ୍ୟ emitCtorDtorDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        ଯଦି (VN_IS(modp, Class)) ଫେରନ୍ତୁ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ଅଚଳ std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    ଶୂନ୍ୟ emitInternalMethodDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ତାର୍କିକ first = ସତ୍ୟ;
        ଅଚଳ string section = "\n// INTERNAL METHODS\n";

        ଯଦି (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            ଯଦି (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } ଅନ୍ୟଥା {
            decorateFirst(first, section);
            ଅଚଳ std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        ଯଦି (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        ଯଦି (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        ଯଦି (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(modp, Class)) {
            ଯଦି (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ଅଚଳ AstClass*, ଅଚଳ AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ଅଚଳ auto hasEnclosingBackPointer = [](ଅଚଳ AstClass* covergroupp) {
                    ଫେରନ୍ତୁ covergroupp->exists([](ଅଚଳ AstVar* ଅଚଳ varp) {
                        ଅଚଳ AstClassRefDType* ଅଚଳ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ଫେରନ୍ତୁ refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ଅଚଳ memberClassp, AstVar* ଅଚଳ varp) {
                        ଅଚଳ AstClassRefDType* ଅଚଳ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ଯଦି (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ଅଚଳ string className = EmitCUtil::prefixNameProtect(classp);
                ଯଦି (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } ଅନ୍ୟଥା {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    ଯଦି (classp->hasRandVarsUpdate()) {
                        ଅଚଳ string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ଅଚଳ updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    ପାଇଁ (ଅଚଳ EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    ଶୂନ୍ୟ emitEnums(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ତାର୍କିକ first = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstTypedef* ଅଚଳ tdefp = VN_CAST(nodep, Typedef);
            ଯଦି (!tdefp) ଜାରି;
            ଯଦି (!tdefp->attrPublic()) ଜାରି;
            ଅଚଳ AstEnumDType* ଅଚଳ edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            ଯଦି (!edtypep) ଜାରି;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            ଯଦି (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } ଅନ୍ୟଥା {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                ପାଇଁ (ଅଚଳ AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(itemp->valuep(), Const)) {
                        ଯଦି (constp->ଅଙ୍କ().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ଜାରି;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->ଅଙ୍କ().emitC());
                    ଯଦି (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    ଶୂନ୍ୟ emitStructDecl(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, Ast_ଗଣ୍ଠି_UOr_Struct_DType* sdtypep,
                        std::set<Ast_ଗଣ୍ଠି_UOr_Struct_DType*>& emitted) {
        ଯଦି (emitted.ଗଣନା_2(sdtypep) > 0) ଫେରନ୍ତୁ;
        emitted.ଭର୍ତ୍ତି(sdtypep);
        ପାଇଁ (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ subp = itemp->getChildStructp();
            ଯଦି (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                ଯଦି (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        ଯଦି (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } ଅନ୍ୟଥା {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ଗଣନା ଶ୍ରେଣୀ AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା get_ଗଣ୍ଠି_Attribute(ଅଚଳ AstMemberDType* itemp, AttributeType type) {
        ଅଚଳ ତାର୍କିକ isArrayType = itemp->dtypep()->isNonPackedArray();
        ବାଛ (type) {
        ସ୍ଥିତି AttributeType::WIDTH: {
            ଯଦି (isArrayType) {
                // For arrays, get innermost element width
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtype = itemp->dtypep();
                ଯେତେବେଳେ (dtype->subDTypep()) dtype = dtype->subDTypep();
                ଫେରନ୍ତୁ dtype->width();
            }
            ଫେରନ୍ତୁ itemp->width();
        }
        ସ୍ଥିତି AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ଫେରନ୍ତୁ isArrayType ? itemp->dtypep()->dimensions(ସତ୍ୟ).second : 0;
        }
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: {
            ଫେରନ୍ତୁ 0;
        }
        }
    }
    template <AttributeType T>
    ଶୂନ୍ୟ emitMemberVector(ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* sdtypep) {
        puts("return {");
        ତାର୍କିକ needComma = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଯଦି (!itemp->isConstrainedRand()) ଜାରି;
            // Comma handling: add before element except first
            ଯଦି (needComma) puts(",\n");
            putns(itemp, std::to_string(get_ଗଣ୍ଠି_Attribute(itemp, T)));
            needComma = ସତ୍ୟ;
        }
        puts("};\n}\n");
    }
    ଶୂନ୍ୟ emitUnpackedUOrSBody(Ast_ଗଣ୍ଠି_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ମିଥ୍ୟା, ମିଥ୍ୟା));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        ଯଦି (sdtypep->isConstrainedRand()) {
            ତାର୍କିକ needComma = ମିଥ୍ୟା;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ଯଦି (!itemp->isConstrainedRand()) ଜାରି;
                ଯଦି (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = ସତ୍ୟ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = ମିଥ୍ୟା;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ଯଦି (!itemp->isConstrainedRand()) ଜାରି;
                ଯଦି (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", ମିଥ୍ୟା, ମିଥ୍ୟା));
                needComma = ସତ୍ୟ;
            }
            puts(">{};\n}\n");

            needComma = ମିଥ୍ୟା;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ଯଦି (!itemp->isConstrainedRand()) ଜାରି;
                ଯଦି (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = ସତ୍ୟ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଯଦି (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, "if (" + itemp->nameProtect() + " < rhs." + itemp->nameProtect()
                             + ") return true;\n");
            putns(itemp, "if (rhs." + itemp->nameProtect() + " < " + itemp->nameProtect()
                             + ") return false;\n");
        }
        puts("return false;\n");
        puts("}\n");
        puts("};\n");
        puts("template <>\n");
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::prefixNameProtect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    ଶୂନ୍ୟ emitVlAssign(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ lhstype, ଅଚଳ Ast_ଗଣ୍ଠି_DType* rhstype,
                      ଅଚଳ std::string& off, ଅଚଳ std::string& lhsdata,
                      ଅଚଳ std::string& rhsdata, ତାର୍କିକ getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        ଯଦି (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } ଅନ୍ୟଥା {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        ଯଦି (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    ଶୂନ୍ୟ emitPackedMember(ଅଚଳ Ast_ଗଣ୍ଠି_DType* parentDtypep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep,
                          ଅଚଳ std::string& fieldname, ଅଚଳ std::string& offset, ତାର୍କିକ getfunc,
                          ଅଚଳ std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        ଯଦି (ଅଚଳ auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ଅଚଳ std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ଅଚଳ std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ଅଚଳ std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } ଅନ୍ୟଥା ଯଦି (VN_IS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଅଚଳ std::string tmp = m_names.get("__Vtmp");
            ଯଦି (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } ଅନ୍ୟଥା {  // Emit `set` func
                ଅଚଳ std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            ଯଦି (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } ଅନ୍ୟଥା {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    ଶୂନ୍ୟ emitPackedUOrSBody(Ast_ଗଣ୍ଠି_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ଅଚଳ AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        ପାଇଁ (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            ଯଦି (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        ପାଇଁ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ମିଥ୍ୟା, ମିଥ୍ୟା, ସତ୍ୟ));
            puts(";\n");
        }

        ଅଚଳ std::string retArgName = m_names.get("__v");
        ଅଚଳ std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        ଯଦି (VN_IS(sdtypep, StructDType)) {
            ପାଇଁ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/ସତ୍ୟ, retArgName);
            }
        } ଅନ୍ୟଥା {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/ସତ୍ୟ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        ଯଦି (VN_IS(sdtypep, StructDType)) {
            ପାଇଁ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/ମିଥ୍ୟା, retArgName);
            }
        } ଅନ୍ୟଥା {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/ମିଥ୍ୟା, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    ଶୂନ୍ୟ emitStructs(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_ଗଣ୍ଠି_UOr_Struct_DType*> emitted;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstTypedef* ଅଚଳ tdefp = VN_CAST(nodep, Typedef);
            ଯଦି (!tdefp) ଜାରି;
            Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), ଗଣ୍ଠି_UOr_Struct_DType);
            ଯଦି (!sdtypep) ଜାରି;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ଅଚଳ ତାର୍କିକ declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            ଯଦି (declaredInOtherModule) ଜାରି;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    ଶୂନ୍ୟ emitFuncDecls(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ତାର୍କିକ inClassBody) {
        std::vector<ଅଚଳ AstCFunc*> funcsp;

        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstCFunc* ଅଚଳ funcp = VN_CAST(nodep, CFunc)) {
                ଯଦି (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ଜାରି;
                ଯଦି (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ଜାରି;
                ଯଦି (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    ଜାରି;
                ଯଦି (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    ଜାରି;
                funcsp.ଠେଲ_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ଅଚଳ Ast_ଗଣ୍ଠି* ap, ଅଚଳ Ast_ଗଣ୍ଠି* bp) {
            ଫେରନ୍ତୁ ap->name() < bp->name();
        });

        ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ funcp : funcsp) {
            ଯଦି (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    ଶୂନ୍ୟ emitAll(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        // Include files required by this AstNodeModule
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(modp, Class)) {
            ପାଇଁ (ଅଚଳ AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::prefixNameProtect(extp->classp()->classOrPackagep())
                                + ".h\"\n");
            }
        }

        // Forward declarations required by this AstNodeModule
        puts("\nclass " + EmitCUtil::symClassName() + ";\n");

        // From `systemc_header
        emitSystemCSection(modp, VSystemCSectionType::HDR);

        emitStructs(modp);

        // Open class body {{{
        puts("\n");
        putns(modp, "class ");
        ଯଦି (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(modp, Class)) {
            puts(" : ");
            ଯଦି (classp->extendsp()) {
                ତାର୍କିକ needComma = ମିଥ୍ୟା;
                ପାଇଁ (ଅଚଳ AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    ଯଦି (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = ସତ୍ୟ;
                }
            } ଅନ୍ୟଥା {
                puts("public virtual VlClass");
            }
        } ଅନ୍ୟଥା {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(ମିଥ୍ୟା);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ ସତ୍ୟ);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ ମିଥ୍ୟା);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::ଶୀର୍ଷ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ଶୀର୍ଷ_Include();
        puts("#include \"verilated.h\"\n");
        ଯଦି (modp->is_ଶୀର୍ଷ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        ଯଦି (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ଯଦି (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ଯଦି (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ଯଦି (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ଯଦି (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        ଯଦି (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        ଯଦି (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.ଭର୍ତ୍ତି(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        ଯଦି (ଅଚଳ AstClassPackage* ଅଚଳ packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        ପାଇଁ (ଅଚଳ string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        ଯଦି (ଅଚଳ AstClassPackage* ଅଚଳ packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ ମୁଖ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) { ଅଚଳ EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

ଶୂନ୍ୟ V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ଯଦି (VN_IS(nodep, Class)) ଜାରି;  // Declared with the ClassPackage
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_AS(nodep, ଗଣ୍ଠି_Module);
        ଯଦି (modp->isConstPool()) ଜାରି;  // Emitted by V3EmitCConstPool
        EmitCHeader::ମୁଖ୍ୟ(modp);
    }
}
