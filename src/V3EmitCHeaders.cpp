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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

ವರ್ಗ EmitCHeader final : ಸಾರ್ವಜನಿಕ EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    ವರ್ಗ Cover_ಎಣಿಕೆ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
        ಪೂರ್ಣಾಙ್ಕ m_bins = 0;  // Running total of coverage bins counted so far

        ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            ಯದಿ (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    ಸಾರ್ವಜನಿಕ:
        explicit Cover_ಎಣಿಕೆ_Visitor(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
            iterateConst(const_cast<Ast_ಗಂಟು_Module*>(modp));
        }
        ಪೂರ್ಣಾಙ್ಕ bins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bins; }
    };

    ಶೂನ್ಯ decorateFirst(ತಾರ್ಕಿಕ& first, ಧ್ರುವ string& str) {
        ಯದಿ (first) {
            putsDecoration(nullptr, str);
            first = ಅಸತ್ಯ;
        }
    }
    ಶೂನ್ಯ emitCellDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ first = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_CAST(nodep, Cell)) {
                ಯದಿ (cellp->modp()->isConstPool()) ಅನುವರ್ತ;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    ಶೂನ್ಯ emitDesignVarDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ಧ್ರುವ AstVar* portp = nullptr;
        ಧ್ರುವ AstVar* trivp = nullptr;
        ಧ್ರುವ AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
            ಯದಿ (!varp) ಭಙ್ಗ;
            ಯದಿ (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ಸಾಲು_Align(), varp, "Aligned non-field");
            } ಅನ್ಯಥಾ ಯದಿ (varp->isPrimaryIO()) {
                ಯದಿ (!portp) portp = varp;
                ++nPort;
            } ಅನ್ಯಥಾ ಯದಿ (varp->isTriviallyConstructible()) {
                ಯದಿ (!trivp) trivp = varp;
                ++nTriv;
            } ಅನ್ಯಥಾ {
                ಯದಿ (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ಧ್ರುವ auto emitAndStep = [&](ಧ್ರುವ AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        ಯದಿ (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            ಪ್ರತಿ (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        ಯದಿ (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            ಯದಿ (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                ಪ್ರತಿ (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                ಯದಿ (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } ಅನ್ಯಥಾ ಯದಿ (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } ಅನ್ಯಥಾ ಯದಿ (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                ಯದಿ (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                ಪ್ರತಿ (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    ಯದಿ (anonL3s != 1) puts("struct {\n");
                    ಪ್ರತಿ (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        ಯದಿ (anonL2s != 1) puts("struct {\n");
                        ಪ್ರತಿ (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            ಯದಿ (anonL1s != 1) puts("struct {\n");
                            ಪ್ರತಿ (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            ಯದಿ (anonL1s != 1) puts("};\n");
                        }
                        ಯದಿ (anonL2s != 1) puts("};\n");
                    }
                    ಯದಿ (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        ಯದಿ (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            ಯದಿ (VN_IS(modp, Class)) {
                ಪ್ರತಿ (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            ಪ್ರತಿ (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    ಶೂನ್ಯ emitInternalVarDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class)) {
            ಯದಿ (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } ಅನ್ಯಥಾ {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ coverBins = Cover_ಎಣಿಕೆ_Visitor{modp}.bins();
            ಯದಿ (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    ಶೂನ್ಯ emitParamDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ first = ಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                ಯದಿ (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), ಅಸತ್ಯ, ಅಸತ್ಯ));
                    puts(";\n");
                }
            }
        }
    }
    ಶೂನ್ಯ emitCtorDtorDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        ಯದಿ (VN_IS(modp, Class)) ಪ್ರತ್ಯಾಗಮ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ಧ್ರುವ std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    ಶೂನ್ಯ emitInternalMethodDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ first = ಸತ್ಯ;
        ಧ್ರುವ string section = "\n// INTERNAL METHODS\n";

        ಯದಿ (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            ಯದಿ (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } ಅನ್ಯಥಾ {
            decorateFirst(first, section);
            ಧ್ರುವ std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        ಯದಿ (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        ಯದಿ (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        ಯದಿ (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class)) {
            ಯದಿ (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ಧ್ರುವ AstClass*, ಧ್ರುವ AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ಧ್ರುವ auto hasEnclosingBackPointer = [](ಧ್ರುವ AstClass* covergroupp) {
                    ಪ್ರತ್ಯಾಗಮ covergroupp->exists([](ಧ್ರುವ AstVar* ಧ್ರುವ varp) {
                        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ಪ್ರತ್ಯಾಗಮ refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ಧ್ರುವ memberClassp, AstVar* ಧ್ರುವ varp) {
                        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        ಯದಿ (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ಧ್ರುವ string className = EmitCUtil::prefixNameProtect(classp);
                ಯದಿ (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } ಅನ್ಯಥಾ {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    ಯದಿ (classp->hasRandVarsUpdate()) {
                        ಧ್ರುವ string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ಧ್ರುವ updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    ಪ್ರತಿ (ಧ್ರುವ EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    ಶೂನ್ಯ emitEnums(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ತಾರ್ಕಿಕ first = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstTypedef* ಧ್ರುವ tdefp = VN_CAST(nodep, Typedef);
            ಯದಿ (!tdefp) ಅನುವರ್ತ;
            ಯದಿ (!tdefp->attrPublic()) ಅನುವರ್ತ;
            ಧ್ರುವ AstEnumDType* ಧ್ರುವ edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            ಯದಿ (!edtypep) ಅನುವರ್ತ;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            ಯದಿ (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } ಅನ್ಯಥಾ {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(itemp->valuep(), Const)) {
                        ಯದಿ (constp->ಅಂಕೆ().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ಅನುವರ್ತ;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->ಅಂಕೆ().emitC());
                    ಯದಿ (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    ಶೂನ್ಯ emitStructDecl(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು_UOr_Struct_DType* sdtypep,
                        std::set<Ast_ಗಂಟು_UOr_Struct_DType*>& emitted) {
        ಯದಿ (emitted.ಎಣಿಕೆ(sdtypep) > 0) ಪ್ರತ್ಯಾಗಮ;
        emitted.ಸೇರಿಸು(sdtypep);
        ಪ್ರತಿ (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp = itemp->getChildStructp();
            ಯದಿ (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                ಯದಿ (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        ಯದಿ (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } ಅನ್ಯಥಾ {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    ಗಣನಾ ವರ್ಗ AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    ಪೂರ್ಣಾಙ್ಕ get_ಗಂಟು_Attribute(ಧ್ರುವ AstMemberDType* itemp, AttributeType type) {
        ಧ್ರುವ ತಾರ್ಕಿಕ isArrayType = itemp->dtypep()->isNonPackedArray();
        ವಿಕಲ್ಪ (type) {
        ಪಕ್ಷ AttributeType::WIDTH: {
            ಯದಿ (isArrayType) {
                // For arrays, get innermost element width
                ಧ್ರುವ Ast_ಗಂಟು_DType* dtype = itemp->dtypep();
                ಯಾವತ್ (dtype->subDTypep()) dtype = dtype->subDTypep();
                ಪ್ರತ್ಯಾಗಮ dtype->width();
            }
            ಪ್ರತ್ಯಾಗಮ itemp->width();
        }
        ಪಕ್ಷ AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            ಪ್ರತ್ಯಾಗಮ isArrayType ? itemp->dtypep()->dimensions(ಸತ್ಯ).second : 0;
        }
        ಸ್ವಭಾವ: {
            ಪ್ರತ್ಯಾಗಮ 0;
        }
        }
    }
    template <AttributeType T>
    ಶೂನ್ಯ emitMemberVector(ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        puts("return {");
        ತಾರ್ಕಿಕ needComma = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಯದಿ (!itemp->isConstrainedRand()) ಅನುವರ್ತ;
            // Comma handling: add before element except first
            ಯದಿ (needComma) puts(",\n");
            putns(itemp, std::to_string(get_ಗಂಟು_Attribute(itemp, T)));
            needComma = ಸತ್ಯ;
        }
        puts("};\n}\n");
    }
    ಶೂನ್ಯ emitUnpackedUOrSBody(Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ಅಸತ್ಯ, ಅಸತ್ಯ));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        ಯದಿ (sdtypep->isConstrainedRand()) {
            ತಾರ್ಕಿಕ needComma = ಅಸತ್ಯ;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಯದಿ (!itemp->isConstrainedRand()) ಅನುವರ್ತ;
                ಯದಿ (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = ಸತ್ಯ;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = ಅಸತ್ಯ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಯದಿ (!itemp->isConstrainedRand()) ಅನುವರ್ತ;
                ಯದಿ (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", ಅಸತ್ಯ, ಅಸತ್ಯ));
                needComma = ಸತ್ಯ;
            }
            puts(">{};\n}\n");

            needComma = ಅಸತ್ಯ;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                ಯದಿ (!itemp->isConstrainedRand()) ಅನುವರ್ತ;
                ಯದಿ (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = ಸತ್ಯ;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಯದಿ (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    ಶೂನ್ಯ emitVlAssign(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhstype, ಧ್ರುವ Ast_ಗಂಟು_DType* rhstype,
                      ಧ್ರುವ std::string& off, ಧ್ರುವ std::string& lhsdata,
                      ಧ್ರುವ std::string& rhsdata, ತಾರ್ಕಿಕ getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        ಯದಿ (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } ಅನ್ಯಥಾ {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        ಯದಿ (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    ಶೂನ್ಯ emitPackedMember(ಧ್ರುವ Ast_ಗಂಟು_DType* parentDtypep, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep,
                          ಧ್ರುವ std::string& fieldname, ಧ್ರುವ std::string& offset, ತಾರ್ಕಿಕ getfunc,
                          ಧ್ರುವ std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (ಧ್ರುವ auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ಧ್ರುವ std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ಧ್ರುವ std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ಧ್ರುವ std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಧ್ರುವ std::string tmp = m_names.get("__Vtmp");
            ಯದಿ (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } ಅನ್ಯಥಾ {  // Emit `set` func
                ಧ್ರುವ std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            ಯದಿ (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } ಅನ್ಯಥಾ {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    ಶೂನ್ಯ emitPackedUOrSBody(Ast_ಗಂಟು_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ಧ್ರುವ AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        ಪ್ರತಿ (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            ಯದಿ (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        ಪ್ರತಿ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), ಅಸತ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ));
            puts(";\n");
        }

        ಧ್ರುವ std::string retArgName = m_names.get("__v");
        ಧ್ರುವ std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        ಯದಿ (VN_IS(sdtypep, StructDType)) {
            ಪ್ರತಿ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/ಸತ್ಯ, retArgName);
            }
        } ಅನ್ಯಥಾ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/ಸತ್ಯ, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        ಯದಿ (VN_IS(sdtypep, StructDType)) {
            ಪ್ರತಿ (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/ಅಸತ್ಯ, retArgName);
            }
        } ಅನ್ಯಥಾ {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/ಅಸತ್ಯ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    ಶೂನ್ಯ emitStructs(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_ಗಂಟು_UOr_Struct_DType*> emitted;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstTypedef* ಧ್ರುವ tdefp = VN_CAST(nodep, Typedef);
            ಯದಿ (!tdefp) ಅನುವರ್ತ;
            Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), ಗಂಟು_UOr_Struct_DType);
            ಯದಿ (!sdtypep) ಅನುವರ್ತ;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ಧ್ರುವ ತಾರ್ಕಿಕ declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            ಯದಿ (declaredInOtherModule) ಅನುವರ್ತ;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    ಶೂನ್ಯ emitFuncDecls(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ inClassBody) {
        std::vector<ಧ್ರುವ AstCFunc*> funcsp;

        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = VN_CAST(nodep, CFunc)) {
                ಯದಿ (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ಅನುವರ್ತ;
                ಯದಿ (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ಅನುವರ್ತ;
                ಯದಿ (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    ಅನುವರ್ತ;
                ಯದಿ (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    ಅನುವರ್ತ;
                funcsp.ತಳ್ಳು_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ಧ್ರುವ Ast_ಗಂಟು* ap, ಧ್ರುವ Ast_ಗಂಟು* bp) {
            ಪ್ರತ್ಯಾಗಮ ap->name() < bp->name();
        });

        ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp : funcsp) {
            ಯದಿ (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    ಶೂನ್ಯ emitAll(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        // Include files required by this AstNodeModule
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class)) {
            ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extp = classp->extendsp(); extp;
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
        ಯದಿ (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class)) {
            puts(" : ");
            ಯದಿ (classp->extendsp()) {
                ತಾರ್ಕಿಕ needComma = ಅಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    ಯದಿ (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = ಸತ್ಯ;
                }
            } ಅನ್ಯಥಾ {
                puts("public virtual VlClass");
            }
        } ಅನ್ಯಥಾ {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(ಅಸತ್ಯ);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ ಸತ್ಯ);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ ಅಸತ್ಯ);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::ತುದಿ_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_ತುದಿ_Include();
        puts("#include \"verilated.h\"\n");
        ಯದಿ (modp->is_ತುದಿ() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        ಯದಿ (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        ಯದಿ (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        ಯದಿ (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        ಯದಿ (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        ಯದಿ (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        ಯದಿ (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        ಯದಿ (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.ಸೇರಿಸು(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        ಯದಿ (ಧ್ರುವ AstClassPackage* ಧ್ರುವ packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        ಪ್ರತಿ (ಧ್ರುವ string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        ಯದಿ (ಧ್ರುವ AstClassPackage* ಧ್ರುವ packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ main(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) { ಧ್ರುವ EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ಯದಿ (VN_IS(nodep, Class)) ಅನುವರ್ತ;  // Declared with the ClassPackage
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_AS(nodep, ಗಂಟು_Module);
        ಯದಿ (modp->isConstPool()) ಅನುವರ್ತ;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
