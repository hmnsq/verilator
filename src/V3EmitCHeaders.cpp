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
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cstdint>
#अन्तर्भाव <set>
#अन्तर्भाव <string>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

वग्ग EmitCHeader final : सार्वजनिक EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    वग्ग Cover_गणनम्_Visitor final : सार्वजनिक VNVisitorConst {
        पूर्णाङ्क m_bins = 0;  // Running total of coverage bins counted so far

        शून्य visit(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            सचे (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    सार्वजनिक:
        explicit Cover_गणनम्_Visitor(ध्रुव Ast_ग्रन्थिः_Module* modp) {
            iterateConst(const_cast<Ast_ग्रन्थिः_Module*>(modp));
        }
        पूर्णाङ्क bins() ध्रुव { पटिनिवत्त m_bins; }
    };

    शून्य decorateFirst(तार्किक& first, ध्रुव string& str) {
        सचे (first) {
            putsDecoration(nullptr, str);
            first = मिच्छा;
        }
    }
    शून्य emitCellDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सच्च;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                सचे (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    शून्य emitDesignVarDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ध्रुव AstVar* portp = nullptr;
        ध्रुव AstVar* trivp = nullptr;
        ध्रुव AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            सचे (!varp) भङ्ग;
            सचे (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_पङ्क्तिः_Align(), varp, "Aligned non-field");
            } अञ्ञथा सचे (varp->isPrimaryIO()) {
                सचे (!portp) portp = varp;
                ++nPort;
            } अञ्ञथा सचे (varp->isTriviallyConstructible()) {
                सचे (!trivp) trivp = varp;
                ++nTriv;
            } अञ्ञथा {
                सचे (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ध्रुव auto emitAndStep = [&](ध्रुव AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        सचे (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            अत्थाय (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        सचे (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            सचे (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                अत्थाय (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } अञ्ञथा {
                ध्रुव size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                सचे (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } अञ्ञथा सचे (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } अञ्ञथा सचे (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                सचे (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                अत्थाय (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    सचे (anonL3s != 1) puts("struct {\n");
                    अत्थाय (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        सचे (anonL2s != 1) puts("struct {\n");
                        अत्थाय (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            सचे (anonL1s != 1) puts("struct {\n");
                            अत्थाय (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            सचे (anonL1s != 1) puts("};\n");
                        }
                        सचे (anonL2s != 1) puts("};\n");
                    }
                    सचे (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        सचे (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            सचे (VN_IS(modp, Class)) {
                अत्थाय (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                पटिनिवत्त;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            अत्थाय (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    शून्य emitInternalVarDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        सचे (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            सचे (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } अञ्ञथा {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ध्रुव पूर्णाङ्क coverBins = Cover_गणनम्_Visitor{modp}.bins();
            सचे (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    शून्य emitParamDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सच्च;
        अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                सचे (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), मिच्छा, मिच्छा));
                    puts(";\n");
                }
            }
        }
    }
    शून्य emitCtorDtorDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        सचे (VN_IS(modp, Class)) पटिनिवत्त;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ध्रुव std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    शून्य emitInternalMethodDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सच्च;
        ध्रुव string section = "\n// INTERNAL METHODS\n";

        सचे (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            सचे (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अञ्ञथा {
            decorateFirst(first, section);
            ध्रुव std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दाः_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        सचे (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        सचे (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        सचे (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        सचे (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            सचे (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ध्रुव AstClass*, ध्रुव AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ध्रुव auto hasEnclosingBackPointer = [](ध्रुव AstClass* covergroupp) {
                    पटिनिवत्त covergroupp->exists([](ध्रुव AstVar* ध्रुव varp) {
                        ध्रुव AstClassRefDType* ध्रुव refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        पटिनिवत्त refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ध्रुव memberClassp, AstVar* ध्रुव varp) {
                        ध्रुव AstClassRefDType* ध्रुव refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        सचे (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ध्रुव string className = EmitCUtil::prefixNameProtect(classp);
                सचे (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } अञ्ञथा {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    सचे (classp->hasRandVarsUpdate()) {
                        ध्रुव string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ध्रुव updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    अत्थाय (ध्रुव EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    शून्य emitEnums(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सच्च;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            सचे (!tdefp) अनुवर्त;
            सचे (!tdefp->attrPublic()) अनुवर्त;
            ध्रुव AstEnumDType* ध्रुव edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            सचे (!edtypep) अनुवर्त;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            सचे (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अञ्ञथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                अत्थाय (ध्रुव AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(itemp->valuep(), Const)) {
                        सचे (constp->अङ्कः().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            अनुवर्त;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अङ्कः().emitC());
                    सचे (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    शून्य emitStructDecl(ध्रुव Ast_ग्रन्थिः_Module* modp, Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep,
                        std::set<Ast_ग्रन्थिः_UOr_Struct_DType*>& emitted) {
        सचे (emitted.गणनम्(sdtypep) > 0) पटिनिवत्त;
        emitted.निवेशय(sdtypep);
        अत्थाय (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp();
            सचे (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                सचे (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        सचे (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } अञ्ञथा {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    गणना वग्ग AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    पूर्णाङ्क get_ग्रन्थिः_Attribute(ध्रुव AstMemberDType* itemp, AttributeType type) {
        ध्रुव तार्किक isArrayType = itemp->dtypep()->isNonPackedArray();
        विकल्प (type) {
        पक्ष AttributeType::WIDTH: {
            सचे (isArrayType) {
                // For arrays, get innermost element width
                ध्रुव Ast_ग्रन्थिः_DType* dtype = itemp->dtypep();
                याव (dtype->subDTypep()) dtype = dtype->subDTypep();
                पटिनिवत्त dtype->width();
            }
            पटिनिवत्त itemp->width();
        }
        पक्ष AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            पटिनिवत्त isArrayType ? itemp->dtypep()->dimensions(सच्च).second : 0;
        }
        स्वभाव: {
            पटिनिवत्त 0;
        }
        }
    }
    template <AttributeType T>
    शून्य emitMemberVector(ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        puts("return {");
        तार्किक needComma = मिच्छा;
        अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            सचे (!itemp->isConstrainedRand()) अनुवर्त;
            // Comma handling: add before element except first
            सचे (needComma) puts(",\n");
            putns(itemp, std::to_string(get_ग्रन्थिः_Attribute(itemp, T)));
            needComma = सच्च;
        }
        puts("};\n}\n");
    }
    शून्य emitUnpackedUOrSBody(Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), मिच्छा, मिच्छा));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        सचे (sdtypep->isConstrainedRand()) {
            तार्किक needComma = मिच्छा;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                सचे (!itemp->isConstrainedRand()) अनुवर्त;
                सचे (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = सच्च;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = मिच्छा;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                सचे (!itemp->isConstrainedRand()) अनुवर्त;
                सचे (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", मिच्छा, मिच्छा));
                needComma = सच्च;
            }
            puts(">{};\n}\n");

            needComma = मिच्छा;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                सचे (!itemp->isConstrainedRand()) अनुवर्त;
                सचे (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = सच्च;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            सचे (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        अत्थाय (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    शून्य emitVlAssign(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhstype, ध्रुव Ast_ग्रन्थिः_DType* rhstype,
                      ध्रुव std::string& off, ध्रुव std::string& lhsdata,
                      ध्रुव std::string& rhsdata, तार्किक getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        सचे (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } अञ्ञथा {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        सचे (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    शून्य emitPackedMember(ध्रुव Ast_ग्रन्थिः_DType* parentDtypep, ध्रुव Ast_ग्रन्थिः_DType* dtypep,
                          ध्रुव std::string& fieldname, ध्रुव std::string& offset, तार्किक getfunc,
                          ध्रुव std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        सचे (ध्रुव auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ध्रुव std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ध्रुव std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ध्रुव std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } अञ्ञथा सचे (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            ध्रुव std::string tmp = m_names.get("__Vtmp");
            सचे (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } अञ्ञथा {  // Emit `set` func
                ध्रुव std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } अञ्ञथा {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            सचे (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } अञ्ञथा {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    शून्य emitPackedUOrSBody(Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ध्रुव AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        अत्थाय (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            सचे (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        अत्थाय (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), मिच्छा, मिच्छा, सच्च));
            puts(";\n");
        }

        ध्रुव std::string retArgName = m_names.get("__v");
        ध्रुव std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        सचे (VN_IS(sdtypep, StructDType)) {
            अत्थाय (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/सच्च, retArgName);
            }
        } अञ्ञथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/सच्च, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        सचे (VN_IS(sdtypep, StructDType)) {
            अत्थाय (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/मिच्छा, retArgName);
            }
        } अञ्ञथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/मिच्छा, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    शून्य emitStructs(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_ग्रन्थिः_UOr_Struct_DType*> emitted;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            सचे (!tdefp) अनुवर्त;
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), ग्रन्थिः_UOr_Struct_DType);
            सचे (!sdtypep) अनुवर्त;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ध्रुव तार्किक declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            सचे (declaredInOtherModule) अनुवर्त;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    शून्य emitFuncDecls(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक inClassBody) {
        std::vector<ध्रुव AstCFunc*> funcsp;

        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
                सचे (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    अनुवर्त;
                सचे (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    अनुवर्त;
                सचे (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    अनुवर्त;
                सचे (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    अनुवर्त;
                funcsp.प्रक्षिप_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ध्रुव Ast_ग्रन्थिः* ap, ध्रुव Ast_ग्रन्थिः* bp) {
            पटिनिवत्त ap->name() < bp->name();
        });

        अत्थाय (ध्रुव AstCFunc* ध्रुव funcp : funcsp) {
            सचे (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    शून्य emitAll(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Include files required by this AstNodeModule
        सचे (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            अत्थाय (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
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
        सचे (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        सचे (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            puts(" : ");
            सचे (classp->extendsp()) {
                तार्किक needComma = मिच्छा;
                अत्थाय (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    सचे (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = सच्च;
                }
            } अञ्ञथा {
                puts("public virtual VlClass");
            }
        } अञ्ञथा {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(मिच्छा);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ सच्च);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ मिच्छा);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::शिखरम्_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_शिखरम्_Include();
        puts("#include \"verilated.h\"\n");
        सचे (modp->is_शिखरम्() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        सचे (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        सचे (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        सचे (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        सचे (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        सचे (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        सचे (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        सचे (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.निवेशय(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        सचे (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        अत्थाय (ध्रुव string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        सचे (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य main(ध्रुव Ast_ग्रन्थिः_Module* modp) { ध्रुव EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        सचे (VN_IS(nodep, Class)) अनुवर्त;  // Declared with the ClassPackage
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(nodep, ग्रन्थिः_Module);
        सचे (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
