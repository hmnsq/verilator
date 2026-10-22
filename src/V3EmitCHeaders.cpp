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

वर्ग EmitCHeader final : सार्वजनिक EmitCBaseVisitorConst {
    V3_Unique_नामानि m_नामानि;
    VMemberMap m_memberMap;
    // METHODS

    वर्ग Cover_गणनम्_Visitor final : सार्वजनिक VNVisitorConst {
        पूर्णाङ्क m_bins = 0;  // Running total of coverage bins counted so far

        शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            यदि (!nodep->दत्त_Decl_Nullp()) m_bins += nodep->आकार();
        }
        शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    सार्वजनिक:
        स्पष्ट Cover_गणनम्_Visitor(ध्रुव Ast_ग्रन्थिः_Module* modp) {
            iterateConst(const_cast<Ast_ग्रन्थिः_Module*>(modp));
        }
        पूर्णाङ्क bins() ध्रुव { प्रत्यागम m_bins; }
    };

    शून्य decorateFirst(तार्किक& first, ध्रुव string& str) {
        यदि (first) {
            putsDecoration(शून्यसूचकः, str);
            first = असत्य;
        }
    }
    शून्य emitCellDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::उपसर्गः_Name_Protect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    शून्य emitDesignVarDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ध्रुव AstVar* portp = शून्यसूचकः;
        ध्रुव AstVar* trivp = शून्यसूचकः;
        ध्रुव AstVar* nontp = शून्यसूचकः;
        आकार_t n_पत्तनम् = 0;
        आकार_t nTriv = 0;
        आकार_t nNont = 0;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (!varp) भङ्ग;
            यदि (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_पङ्क्तिः_Align(), varp, "Aligned non-field");
            } अन्यथा यदि (varp->isPrimaryIO()) {
                यदि (!portp) portp = varp;
                ++n_पत्तनम्;
            } अन्यथा यदि (varp->isTriviallyConstructible()) {
                यदि (!trivp) trivp = varp;
                ++nTriv;
            } अन्यथा {
                यदि (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ध्रुव स्वचालित emit_And_चरणः = [&](ध्रुव AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        यदि (n_पत्तनम्) {
            putsDecoration(शून्यसूचकः, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            पङ्क्तिं_लिख("union {\nstruct {\n");
            प्रति (आकार_t i = 0; i < n_पत्तनम्; ++i) emit_And_चरणः(portp);
            पङ्क्तिं_लिख("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        यदि (nTriv) {
            putsDecoration(शून्यसूचकः, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            यदि (v3Global.opt.comp_सीमा_Members() == 0) {
                // Anonymous structs disabled, so just emit them directly
                प्रति (आकार_t i = 0; i < nTriv; ++i) emit_And_चरणः(trivp);
            } अन्यथा {
                ध्रुव आकार_t lim = v3Global.opt.comp_सीमा_Members();
                आकार_t anonL3s = 1;
                आकार_t anonL2s = 1;
                आकार_t anonL1s = 1;
                यदि (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } अन्यथा यदि (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } अन्यथा यदि (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                यदि (anonL1s != 1) {
                    putsDecoration(
                        शून्यसूचकः,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                आकार_t nEmitted = 0;
                प्रति (आकार_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    यदि (anonL3s != 1) पङ्क्तिं_लिख("struct {\n");
                    प्रति (आकार_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        यदि (anonL2s != 1) पङ्क्तिं_लिख("struct {\n");
                        प्रति (आकार_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            यदि (anonL1s != 1) पङ्क्तिं_लिख("struct {\n");
                            प्रति (आकार_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emit_And_चरणः(trivp);
                            }
                            यदि (anonL1s != 1) पङ्क्तिं_लिख("};\n");
                        }
                        यदि (anonL2s != 1) पङ्क्तिं_लिख("};\n");
                    }
                    यदि (anonL3s != 1) पङ्क्तिं_लिख("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        यदि (nNont) {
            putsDecoration(शून्यसूचकः, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            यदि (VN_IS(modp, Class)) {
                प्रति (आकार_t i = 0; i < nNont; ++i) emit_And_चरणः(nontp);
                प्रत्यागम;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            पङ्क्तिं_लिख("union {\nstruct {\n");
            प्रति (आकार_t i = 0; i < nNont; ++i) emit_And_चरणः(nontp);
            पङ्क्तिं_लिख("} __Vnontrivs;\n};\n");
        }
    }
    शून्य emit_आन्तरिकम्_Var_Decls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            यदि (classp->needRNG()) {
                putsDecoration(शून्यसूचकः, "\n// INTERNAL VARIABLES\n");
                पङ्क्तिं_लिख("VlRNGReseeds __Vm_rng;\n");
            }
        } अन्यथा {  // not class
            putsDecoration(शून्यसूचकः, "\n// INTERNAL VARIABLES\n");
            पङ्क्तिं_लिख(EmitCUtil::symClassName() + "* vlSymsp;\n");
            पङ्क्तिं_लिख("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ध्रुव पूर्णाङ्क coverBins = Cover_गणनम्_Visitor{modp}.bins();
            यदि (coverBins) {
                पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                पङ्क्तिं_लिख(" __Vcoverage[");
                पङ्क्तिं_लिख(std::गन्तव्यम्_string(coverBins));
                पङ्क्तिं_लिख("]{};\n");
            }
        }
    }
    शून्य emitParamDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सत्य;
        प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    पङ्क्तिं_लिख(varp->dtypep()->cType(varp->nameProtect(), असत्य, असत्य));
                    पङ्क्तिं_लिख(";\n");
                }
            }
        }
    }
    शून्य emitCtorDtorDecls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        यदि (VN_IS(modp, Class)) प्रत्यागम;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ध्रुव std::string name = EmitCUtil::उपसर्गः_Name_Protect(modp);
        putsDecoration(शून्यसूचकः, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    शून्य emit_आन्तरिकम्_विधानम्_Decls(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सत्य;
        ध्रुव string section = "\n// INTERNAL METHODS\n";

        यदि (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            यदि (v3Global.opt.coverage()) {
                पङ्क्तिं_लिख("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अन्यथा {
            decorateFirst(first, section);
            ध्रुव std::string name = V3_बहिर्गमनम्_Formatter::quoteNameControls(
                VIdProtect::protect_शब्दाः_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            पङ्क्तिं_लिख("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        यदि (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            पङ्क्तिं_लिख("void __vlCoverInsert(");
            पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            पङ्क्तिं_लिख("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            पङ्क्तिं_लिख("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            पङ्क्तिं_लिख("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        यदि (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            पङ्क्तिं_लिख("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            पङ्क्तिं_लिख("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            पङ्क्तिं_लिख("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        यदि (v3Global.opt.savable()) {
            decorateFirst(first, section);
            पङ्क्तिं_लिख("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            पङ्क्तिं_लिख("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            यदि (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                उपयुज्य EmbeddedCovergroupVar = std::pair<ध्रुव AstClass*, ध्रुव AstVar*>;
                std::सदिशम्<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ध्रुव स्वचालित has_Enclosing_पृष्ठभागः_Pointer = [](ध्रुव AstClass* covergroupp) {
                    प्रत्यागम covergroupp->exists([](ध्रुव AstVar* ध्रुव varp) {
                        ध्रुव AstClassRefDType* ध्रुव refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        प्रत्यागम refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ध्रुव memberClassp, AstVar* ध्रुव varp) {
                        ध्रुव AstClassRefDType* ध्रुव refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        यदि (refp && refp->classp()->isCovergroup()
                            && has_Enclosing_पृष्ठभागः_Pointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_पृष्ठभागः(memberClassp, varp);
                        }
                    });
                ध्रुव string className = EmitCUtil::उपसर्गः_Name_Protect(classp);
                यदि (embeddedCovergroupVars.रिक्तानि() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } अन्यथा {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    यदि (classp->hasRandVarsUpdate()) {
                        ध्रुव string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ध्रुव updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        पङ्क्तिं_लिख("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    प्रति (ध्रुव EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        पङ्क्तिं_लिख("clonep->" + EmitCUtil::उपसर्गः_Name_Protect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    पङ्क्तिं_लिख("return clonep; }\n");
                }
            }
        }
    }
    शून्य emitEnums(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) अनुवर्त;
            यदि (!tdefp->attr_लौकिकम्()) अनुवर्त;
            ध्रुव AstEnumDType* ध्रुव edtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
            यदि (!edtypep) अनुवर्त;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            यदि (edtypep->विस्तारः() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अन्यथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                प्रति (ध्रुव AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(itemp->valuep(), Const)) {
                        यदि (constp->अङ्कः().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            अनुवर्त;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    पङ्क्तिं_लिख(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अङ्कः().emitC());
                    यदि (VN_IS(itemp->nextp(), EnumItem)) पङ्क्तिं_लिख(",");
                    पङ्क्तिं_लिख("\n");
                }
                पङ्क्तिं_लिख("};\n");
            }
        }
    }
    शून्य emitStructDecl(ध्रुव Ast_ग्रन्थिः_Module* modp, Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep,
                        std::समुच्चयः<Ast_ग्रन्थिः_UOr_Struct_DType*>& emitted) {
        यदि (emitted.गणनम्(sdtypep) > 0) प्रत्यागम;
        emitted.निवेशय(sdtypep);
        प्रति (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp();
            यदि (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                यदि (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    पङ्क्तिं_लिख("\n");
                }
            }
        }
        यदि (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } अन्यथा {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    गणना वर्ग AttributeType : uint8_t { विस्तारः_2, DIMENSION };
    // Get member attribute based on type
    पूर्णाङ्क get_ग्रन्थिः_Attribute(ध्रुव AstMemberDType* itemp, AttributeType type) {
        ध्रुव तार्किक isArrayType = itemp->dtypep()->isNonPackedArray();
        विकल्प (type) {
        पक्ष AttributeType::विस्तारः_2: {
            यदि (isArrayType) {
                // For arrays, get innermost element width
                ध्रुव Ast_ग्रन्थिः_DType* dtype = itemp->dtypep();
                यावत् (dtype->subDTypep()) dtype = dtype->subDTypep();
                प्रत्यागम dtype->विस्तारः();
            }
            प्रत्यागम itemp->विस्तारः();
        }
        पक्ष AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            प्रत्यागम isArrayType ? itemp->dtypep()->dimensions(सत्य).second : 0;
        }
        स्वभाव: {
            प्रत्यागम 0;
        }
        }
    }
    प्रतिरूपम् <AttributeType T>
    शून्य emit_Member_सदिशम्(ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        पङ्क्तिं_लिख("return {");
        तार्किक needComma = असत्य;
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (!itemp->isConstrainedRand()) अनुवर्त;
            // Comma handling: add before element except first
            यदि (needComma) पङ्क्तिं_लिख(",\n");
            putns(itemp, std::गन्तव्यम्_string(get_ग्रन्थिः_Attribute(itemp, T)));
            needComma = सत्य;
        }
        पङ्क्तिं_लिख("};\n}\n");
    }
    शून्य emitUnpackedUOrSBody(Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        पङ्क्तिं_लिख(" " + EmitCUtil::उपसर्गः_Name_Protect(sdtypep) + " {\n");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य));
            पङ्क्तिं_लिख(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        यदि (sdtypep->isConstrainedRand()) {
            तार्किक needComma = असत्य;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            पङ्क्तिं_लिख("return {");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) पङ्क्तिं_लिख(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = सत्य;
            }
            पङ्क्तिं_लिख("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emit_Member_सदिशम्<AttributeType::विस्तारः_2>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emit_Member_सदिशम्<AttributeType::DIMENSION>(sdtypep);

            needComma = असत्य;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            पङ्क्तिं_लिख("return std::index_sequence_for<");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) पङ्क्तिं_लिख(",\n");
                putns(itemp, itemp->dtypep()->cType("", असत्य, असत्य));
                needComma = सत्य;
            }
            पङ्क्तिं_लिख(">{};\n}\n");

            needComma = असत्य;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            पङ्क्तिं_लिख("return std::tie(");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) पङ्क्तिं_लिख(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = सत्य;
            }
            पङ्क्तिं_लिख(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::उपसर्गः_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        पङ्क्तिं_लिख("return ");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (itemp != sdtypep->membersp()) पङ्क्तिं_लिख("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        पङ्क्तिं_लिख(";\n");
        पङ्क्तिं_लिख("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::उपसर्गः_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        पङ्क्तिं_लिख("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::उपसर्गः_Name_Protect(sdtypep)
                           + "& rhs) const {\n");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, "if (" + itemp->nameProtect() + " < rhs." + itemp->nameProtect()
                             + ") return true;\n");
            putns(itemp, "if (rhs." + itemp->nameProtect() + " < " + itemp->nameProtect()
                             + ") return false;\n");
        }
        पङ्क्तिं_लिख("return false;\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("};\n");
        पङ्क्तिं_लिख("template <>\n");
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::उपसर्गः_Name_Protect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    शून्य emit_Vl_नियोजय(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhstype, ध्रुव Ast_ग्रन्थिः_DType* rhstype,
                      ध्रुव std::string& off, ध्रुव std::string& lhsdata,
                      ध्रुव std::string& rhsdata, तार्किक getfunc) {
        पङ्क्तिं_लिख(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        पङ्क्तिं_लिख(lhstype->charIQWN());
        पङ्क्तिं_लिख(rhstype->charIQWN());
        पङ्क्तिं_लिख("(" + std::गन्तव्यम्_string(lhstype->विस्तारः()) + ", ");  // LHS width
        यदि (getfunc) {
            पङ्क्तिं_लिख(std::गन्तव्यम्_string(rhstype->विस्तारः()) + ", ");  // Number of copy bits
            पङ्क्तिं_लिख(off + ", ");  // LHS offset
        } अन्यथा {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            पङ्क्तिं_लिख(std::गन्तव्यम्_string(lhstype->विस्तारः_योग_अष्टकानि() * 8) + ", ");
        }
        पङ्क्तिं_लिख(lhsdata + ", ");  // LHS data
        पङ्क्तिं_लिख(rhsdata);  // RHS data
        यदि (!getfunc) {
            पङ्क्तिं_लिख(", " + off);  // RHS offset
        }
        पङ्क्तिं_लिख(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    शून्य emitPackedMember(ध्रुव Ast_ग्रन्थिः_DType* parentDtypep, ध्रुव Ast_ग्रन्थिः_DType* dtypep,
                          ध्रुव std::string& fieldname, ध्रुव std::string& विस्थापनम्, तार्किक getfunc,
                          ध्रुव std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        यदि (ध्रुव स्वचालित* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ध्रुव std::string सूचकांक = m_नामानि.get("__Vi");
            पङ्क्तिं_लिख("for (int " + सूचकांक + " = 0; " + सूचकांक + " < "
                 + std::गन्तव्यम्_string(adtypep->elementsConst()) + "; ++" + सूचकांक + ") {\n");

            ध्रुव std::string विस्थापनम्_अन्तर्_Loop
                = विस्थापनम् + " + " + सूचकांक + " * " + std::गन्तव्यम्_string(adtypep->subDTypep()->विस्तारः());
            ध्रुव std::string newName = fieldname + "[" + सूचकांक + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, विस्थापनम्_अन्तर्_Loop, getfunc,
                             retOrArg);
            पङ्क्तिं_लिख("}\n");
        } अन्यथा यदि (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            ध्रुव std::string tmp = m_नामानि.get("__Vtmp");
            यदि (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                पङ्क्तिं_लिख("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_नियोजय(parentDtypep, dtypep, विस्थापनम्, retOrArg, tmp, getfunc);
            } अन्यथा {  // Emit `set` func
                ध्रुव std::string tmptype = AstCDType::type_गन्तव्यम्_Hold(dtypep->विस्तारः());
                // type tmp;
                पङ्क्तिं_लिख(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_नियोजय(dtypep, parentDtypep, विस्थापनम्, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                पङ्क्तिं_लिख(fieldname + ".set(" + tmp + ");\n");
            }
        } अन्यथा {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            यदि (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emit_Vl_नियोजय(parentDtypep, dtypep, विस्थापनम्, retOrArg, fieldname, getfunc);
            } अन्यथा {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emit_Vl_नियोजय(dtypep, parentDtypep, विस्थापनम्, fieldname, retOrArg, getfunc);
            }
        }
    }
    शून्य emitPackedUOrSBody(Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        पङ्क्तिं_लिख(" " + EmitCUtil::उपसर्गः_Name_Protect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* चरमः_Itemp;
        ध्रुव AstMemberDType* witemp = शून्यसूचकः;
        // LSB is first field in C, so loop backwards
        प्रति (चरमः_Itemp = sdtypep->membersp(); चरमः_Itemp && चरमः_Itemp->nextp();
             चरमः_Itemp = VN_AS(चरमः_Itemp->nextp(), MemberDType)) {
            यदि (चरमः_Itemp->विस्तारः() == sdtypep->विस्तारः()) witemp = चरमः_Itemp;
        }
        प्रति (itemp = चरमः_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य, सत्य));
            पङ्क्तिं_लिख(";\n");
        }

        ध्रुव std::string retArgName = m_नामानि.get("__v");
        ध्रुव std::string retArgType = AstCDType::type_गन्तव्यम्_Hold(sdtypep->विस्तारः());

        // Emit `get` member function
        पङ्क्तिं_लिख(retArgType + " get() const {\n");
        पङ्क्तिं_लिख(retArgType + " " + retArgName + ";\n");
        यदि (VN_IS(sdtypep, StructDType)) {
            प्रति (itemp = चरमः_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::गन्तव्यम्_string(itemp->lsb()), /*getfunc=*/सत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::गन्तव्यम्_string(witemp->lsb()), /*getfunc=*/सत्य, retArgName);
        }
        पङ्क्तिं_लिख("return " + retArgName + ";\n");
        पङ्क्तिं_लिख("}\n");

        // Emit `set` member function
        पङ्क्तिं_लिख("void set(const " + retArgType + "& " + retArgName + ") {\n");
        यदि (VN_IS(sdtypep, StructDType)) {
            प्रति (itemp = चरमः_Itemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::गन्तव्यम्_string(itemp->lsb()), /*getfunc=*/असत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::गन्तव्यम्_string(witemp->lsb()), /*getfunc=*/असत्य, retArgName);
        }

        पङ्क्तिं_लिख("}\n");

        पङ्क्तिं_लिख("};\n");
        m_नामानि.reset();
    }
    शून्य emitStructs(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Track structs that've been emitted already
        std::समुच्चयः<Ast_ग्रन्थिः_UOr_Struct_DType*> emitted;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) अनुवर्त;
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                = VN_CAST(tdefp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), ग्रन्थिः_UOr_Struct_DType);
            यदि (!sdtypep) अनुवर्त;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ध्रुव तार्किक declared_अन्तर्_Other_Module = sdtypep->classOrPackagep() != modp;
            यदि (declared_अन्तर्_Other_Module) अनुवर्त;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    शून्य emitFuncDecls(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक अन्तर्_Class_Body) {
        std::सदिशम्<ध्रुव AstCFunc*> funcsp;

        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
                यदि (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    अनुवर्त;
                यदि (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    अनुवर्त;
                यदि (funcp->is_विधानम्() != अन्तर्_Class_Body)  // Only methods go inside class
                    अनुवर्त;
                यदि (funcp->is_विधानम्() && funcp->isLoose())  // Loose methods are declared lazily
                    अनुवर्त;
                funcsp.प्रक्षिप_पृष्ठभागः(funcp);
            }
        }

        std::stable_क्रमय(funcsp.begin(), funcsp.अन्तः_2(), [](ध्रुव Ast_ग्रन्थिः* ap, ध्रुव Ast_ग्रन्थिः* bp) {
            प्रत्यागम ap->name() < bp->name();
        });

        प्रति (ध्रुव AstCFunc* ध्रुव funcp : funcsp) {
            यदि (अन्तर्_Class_Body) ofp()->puts_वैयक्तिकम्(funcp->decl_वैयक्तिकम्());
            emitCFuncDecl(funcp, modp);
        }
    }
    शून्य emitAll(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Include files required by this AstNodeModule
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            प्रति (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::उपसर्गः_Name_Protect(extp->classp()->classOrPackagep())
                                + ".h\"\n");
            }
        }

        // Forward declarations required by this AstNodeModule
        पङ्क्तिं_लिख("\nclass " + EmitCUtil::symClassName() + ";\n");

        // From `systemc_header
        emitSystemCSection(modp, VSystemCSectionType::HDR);

        emitStructs(modp);

        // Open class body {{{
        पङ्क्तिं_लिख("\n");
        putns(modp, "class ");
        यदि (!VN_IS(modp, Class)) पङ्क्तिं_लिख("alignas(VL_CACHE_LINE_BYTES) ");
        पङ्क्तिं_लिख(EmitCUtil::उपसर्गः_Name_Protect(modp));
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            पङ्क्तिं_लिख(" : ");
            यदि (classp->extendsp()) {
                तार्किक needComma = असत्य;
                प्रति (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    यदि (needComma) पङ्क्तिं_लिख(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    पङ्क्तिं_लिख(extp->classp()->use_Virtual_लौकिकम्() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::उपसर्गः_Name_Protect(extp->classp()));
                    needComma = सत्य;
                }
            } अन्यथा {
                पङ्क्तिं_लिख("public virtual VlClass");
            }
        } अन्यथा {
            पङ्क्तिं_लिख(" final");
        }
        पङ्क्तिं_लिख(" {\n");
        ofp()->reset_वैयक्तिकम्();
        ofp()->puts_वैयक्तिकम्(असत्य);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emit_आन्तरिकम्_Var_Decls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emit_आन्तरिकम्_विधानम्_Decls(modp);
        emitFuncDecls(modp, /* inClassBody: */ सत्य);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        पङ्क्तिं_लिख("};\n");
        // }}}

        // Emit out of class function declarations
        पङ्क्तिं_लिख("\n");
        emitFuncDecls(modp, /* inClassBody: */ असत्य);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    स्पष्ट EmitCHeader(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::उपसर्गः_Name_Protect(modp));

        // Open output file
        open_New_Output_शीर्षकम्_File(EmitCUtil::उपसर्गः_Name_Protect(modp), "Design internal header");
        पङ्क्तिं_लिख("// See " + EmitCUtil::शिखरम्_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        पङ्क्तिं_लिख("\n");
        ofp()->puts_Int_शिखरम्_Include();
        पङ्क्तिं_लिख("#include \"verilated.h\"\n");
        यदि (modp->is_शिखरम्() && optSystemC()) पङ्क्तिं_लिख("#include \"verilated_sc.h\"\n");
        यदि (v3Global.opt.mtasks()) पङ्क्तिं_लिख("#include \"verilated_threads.h\"\n");
        यदि (v3Global.opt.savable()) पङ्क्तिं_लिख("#include \"verilated_save.h\"\n");
        यदि (v3Global.opt.coverage()) पङ्क्तिं_लिख("#include \"verilated_cov.h\"\n");
        यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
            पङ्क्तिं_लिख("#include \"verilated_covergroup.h\"\n");
        यदि (v3Global.उपयुङ्क्ते_Timing()) पङ्क्तिं_लिख("#include \"verilated_timing.h\"\n");
        यदि (v3Global.useRandomizeMethods()) पङ्क्तिं_लिख("#include \"verilated_random.h\"\n");
        यदि (v3Global.उपयुङ्क्ते_Force()) पङ्क्तिं_लिख("#include \"verilated_force.h\"\n");

        std::समुच्चयः<string> cuse_समुच्चयः;
        स्वचालित सङ्कलय_गन्तव्यम्_cuse_समुच्चयः = [&](string s) { cuse_समुच्चयः.निवेशय(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, सङ्कलय_गन्तव्यम्_cuse_समुच्चयः);
        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       सङ्कलय_गन्तव्यम्_cuse_समुच्चयः);
        }

        प्रति (ध्रुव string& s : cuse_समुच्चयः) पङ्क्तिं_लिख(s);
        पङ्क्तिं_लिख("\n");

        emitAll(modp);

        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->puts_अन्तः_Guard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य मुख्यम्(ध्रुव Ast_ग्रन्थिः_Module* modp) { ध्रुव EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitc_शीर्षकाणि() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    प्रति (ध्रुव Ast_ग्रन्थिः* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        यदि (VN_IS(nodep, Class)) अनुवर्त;  // Declared with the ClassPackage
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(nodep, ग्रन्थिः_Module);
        यदि (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
        EmitCHeader::मुख्यम्(modp);
    }
}
