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
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    वर्ग Cover_मोजणी_Visitor final : सार्वजनिक VNVisitorConst {
        पूर्णांक m_bins = 0;  // Running total of coverage bins counted so far

        शून्य visit(Ast_गाठ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            जर (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    सार्वजनिक:
        explicit Cover_मोजणी_Visitor(स्थिरांक Ast_गाठ_Module* modp) {
            iterateConst(const_cast<Ast_गाठ_Module*>(modp));
        }
        पूर्णांक bins() स्थिरांक { परत m_bins; }
    };

    शून्य decorateFirst(तार्किक& first, स्थिरांक string& str) {
        जर (first) {
            putsDecoration(nullptr, str);
            first = असत्य;
        }
    }
    शून्य emitCellDecls(स्थिरांक Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        साठी (स्थिरांक Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstCell* स्थिरांक cellp = VN_CAST(nodep, Cell)) {
                जर (cellp->modp()->isConstPool()) सुरूठेवा;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    शून्य emitDesignVarDecls(स्थिरांक Ast_गाठ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        स्थिरांक AstVar* portp = nullptr;
        स्थिरांक AstVar* trivp = nullptr;
        स्थिरांक AstVar* nontp = nullptr;
        माप_प्रकार nPort = 0;
        माप_प्रकार nTriv = 0;
        माप_प्रकार nNont = 0;
        साठी (स्थिरांक Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var);
            जर (!varp) थांबा;
            जर (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ओळ_Align(), varp, "Aligned non-field");
            } अन्यथा जर (varp->isPrimaryIO()) {
                जर (!portp) portp = varp;
                ++nPort;
            } अन्यथा जर (varp->isTriviallyConstructible()) {
                जर (!trivp) trivp = varp;
                ++nTriv;
            } अन्यथा {
                जर (!nontp) nontp = varp;
                ++nNont;
            }
        }

        स्थिरांक स्वतः emitAndStep = [&](स्थिरांक AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        जर (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            माला_दो("union {\nstruct {\n");
            साठी (माप_प्रकार i = 0; i < nPort; ++i) emitAndStep(portp);
            माला_दो("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        जर (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            जर (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                साठी (माप_प्रकार i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } अन्यथा {
                स्थिरांक माप_प्रकार lim = v3Global.opt.compLimitMembers();
                माप_प्रकार anonL3s = 1;
                माप_प्रकार anonL2s = 1;
                माप_प्रकार anonL1s = 1;
                जर (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } अन्यथा जर (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } अन्यथा जर (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                जर (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                माप_प्रकार nEmitted = 0;
                साठी (माप_प्रकार l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    जर (anonL3s != 1) माला_दो("struct {\n");
                    साठी (माप_प्रकार l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        जर (anonL2s != 1) माला_दो("struct {\n");
                        साठी (माप_प्रकार l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            जर (anonL1s != 1) माला_दो("struct {\n");
                            साठी (माप_प्रकार l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            जर (anonL1s != 1) माला_दो("};\n");
                        }
                        जर (anonL2s != 1) माला_दो("};\n");
                    }
                    जर (anonL3s != 1) माला_दो("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        जर (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            जर (VN_IS(modp, Class)) {
                साठी (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
                परत;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            माला_दो("union {\nstruct {\n");
            साठी (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
            माला_दो("} __Vnontrivs;\n};\n");
        }
    }
    शून्य emitInternalVarDecls(स्थिरांक Ast_गाठ_Module* modp) {
        जर (स्थिरांक AstClass* स्थिरांक classp = VN_CAST(modp, Class)) {
            जर (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                माला_दो("VlRNGReseeds __Vm_rng;\n");
            }
        } अन्यथा {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            माला_दो(EmitCUtil::symClassName() + "* vlSymsp;\n");
            माला_दो("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            स्थिरांक पूर्णांक coverBins = Cover_मोजणी_Visitor{modp}.bins();
            जर (coverBins) {
                माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                माला_दो(" __Vcoverage[");
                माला_दो(std::to_string(coverBins));
                माला_दो("]{};\n");
            }
        }
    }
    शून्य emitParamDecls(स्थिरांक Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        साठी (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
                जर (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    माला_दो(varp->dtypep()->cType(varp->nameProtect(), असत्य, असत्य));
                    माला_दो(";\n");
                }
            }
        }
    }
    शून्य emitCtorDtorDecls(स्थिरांक Ast_गाठ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        जर (VN_IS(modp, Class)) परत;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        स्थिरांक std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    शून्य emitInternalMethodDecls(स्थिरांक Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        स्थिरांक string section = "\n// INTERNAL METHODS\n";

        जर (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            जर (v3Global.opt.coverage()) {
                माला_दो("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अन्यथा {
            decorateFirst(first, section);
            स्थिरांक std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्द_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            माला_दो("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        जर (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            माला_दो("void __vlCoverInsert(");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            माला_दो("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp);\n");
        }

        जर (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            माला_दो("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        जर (v3Global.opt.savable()) {
            decorateFirst(first, section);
            माला_दो("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            माला_दो("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        जर (स्थिरांक AstClass* स्थिरांक classp = VN_CAST(modp, Class)) {
            जर (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<स्थिरांक AstClass*, स्थिरांक AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                स्थिरांक स्वतः hasEnclosingBackPointer = [](स्थिरांक AstClass* covergroupp) {
                    परत covergroupp->exists([](स्थिरांक AstVar* स्थिरांक varp) {
                        स्थिरांक AstClassRefDType* स्थिरांक refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        परत refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* स्थिरांक memberClassp, AstVar* स्थिरांक varp) {
                        स्थिरांक AstClassRefDType* स्थिरांक refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        जर (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                स्थिरांक string className = EmitCUtil::prefixNameProtect(classp);
                जर (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } अन्यथा {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    जर (classp->hasRandVarsUpdate()) {
                        स्थिरांक string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* स्थिरांक updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        माला_दो("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    साठी (स्थिरांक EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        माला_दो("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    माला_दो("return clonep; }\n");
                }
            }
        }
    }
    शून्य emitEnums(स्थिरांक Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        साठी (स्थिरांक Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक AstTypedef* स्थिरांक tdefp = VN_CAST(nodep, Typedef);
            जर (!tdefp) सुरूठेवा;
            जर (!tdefp->attrPublic()) सुरूठेवा;
            स्थिरांक AstEnumDType* स्थिरांक edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            जर (!edtypep) सुरूठेवा;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            जर (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अन्यथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                साठी (स्थिरांक AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(itemp->valuep(), Const)) {
                        जर (constp->अंक().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            सुरूठेवा;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    माला_दो(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अंक().emitC());
                    जर (VN_IS(itemp->nextp(), EnumItem)) माला_दो(",");
                    माला_दो("\n");
                }
                माला_दो("};\n");
            }
        }
    }
    शून्य emitStructDecl(स्थिरांक Ast_गाठ_Module* modp, Ast_गाठ_UOr_Struct_DType* sdtypep,
                        std::set<Ast_गाठ_UOr_Struct_DType*>& emitted) {
        जर (emitted.मोजणी(sdtypep) > 0) परत;
        emitted.घाला(sdtypep);
        साठी (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाठ_UOr_Struct_DType* स्थिरांक subp = itemp->getChildStructp();
            जर (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                जर (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    माला_दो("\n");
                }
            }
        }
        जर (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } अन्यथा {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    गणना वर्ग AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    पूर्णांक get_गाठ_Attribute(स्थिरांक AstMemberDType* itemp, AttributeType type) {
        स्थिरांक तार्किक isArrayType = itemp->dtypep()->isNonPackedArray();
        निवड (type) {
        स्थिती AttributeType::WIDTH: {
            जर (isArrayType) {
                // For arrays, get innermost element width
                स्थिरांक Ast_गाठ_DType* dtype = itemp->dtypep();
                पर्यंत (dtype->subDTypep()) dtype = dtype->subDTypep();
                परत dtype->width();
            }
            परत itemp->width();
        }
        स्थिती AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            परत isArrayType ? itemp->dtypep()->dimensions(सत्य).second : 0;
        }
        पूर्वनिर्धारित: {
            परत 0;
        }
        }
    }
    ढाँचा <AttributeType T>
    शून्य emitMemberVector(स्थिरांक Ast_गाठ_UOr_Struct_DType* sdtypep) {
        माला_दो("return {");
        तार्किक needComma = असत्य;
        साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            जर (!itemp->isConstrainedRand()) सुरूठेवा;
            // Comma handling: add before element except first
            जर (needComma) माला_दो(",\n");
            putns(itemp, std::to_string(get_गाठ_Attribute(itemp, T)));
            needComma = सत्य;
        }
        माला_दो("};\n}\n");
    }
    शून्य emitUnpackedUOrSBody(Ast_गाठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य));
            माला_दो(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        जर (sdtypep->isConstrainedRand()) {
            तार्किक needComma = असत्य;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            माला_दो("return {");
            साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                जर (!itemp->isConstrainedRand()) सुरूठेवा;
                जर (needComma) माला_दो(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = सत्य;
            }
            माला_दो("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = असत्य;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            माला_दो("return std::index_sequence_for<");
            साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                जर (!itemp->isConstrainedRand()) सुरूठेवा;
                जर (needComma) माला_दो(",\n");
                putns(itemp, itemp->dtypep()->cType("", असत्य, असत्य));
                needComma = सत्य;
            }
            माला_दो(">{};\n}\n");

            needComma = असत्य;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            माला_दो("return std::tie(");
            साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                जर (!itemp->isConstrainedRand()) सुरूठेवा;
                जर (needComma) माला_दो(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = सत्य;
            }
            माला_दो(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return ");
        साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            जर (itemp != sdtypep->membersp()) माला_दो("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        माला_दो(";\n");
        माला_दो("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        साठी (स्थिरांक AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, "if (" + itemp->nameProtect() + " < rhs." + itemp->nameProtect()
                             + ") return true;\n");
            putns(itemp, "if (rhs." + itemp->nameProtect() + " < " + itemp->nameProtect()
                             + ") return false;\n");
        }
        माला_दो("return false;\n");
        माला_दो("}\n");
        माला_दो("};\n");
        माला_दो("template <>\n");
        putns(sdtypep, "struct VlIsCustomStruct<" + EmitCUtil::prefixNameProtect(sdtypep)
                           + "> : public std::true_type {};\n");
    }

    // getfunc: VL_ASSIGNSEL_XX(rbits, obits, off, lhsdata, rhsdata);
    // !getfunc: VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, off);
    शून्य emitVlAssign(स्थिरांक Ast_गाठ_DType* स्थिरांक lhstype, स्थिरांक Ast_गाठ_DType* rhstype,
                      स्थिरांक std::string& off, स्थिरांक std::string& lhsdata,
                      स्थिरांक std::string& rhsdata, तार्किक getfunc) {
        माला_दो(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        माला_दो(lhstype->charIQWN());
        माला_दो(rhstype->charIQWN());
        माला_दो("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        जर (getfunc) {
            माला_दो(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            माला_दो(off + ", ");  // LHS offset
        } अन्यथा {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            माला_दो(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        माला_दो(lhsdata + ", ");  // LHS data
        माला_दो(rhsdata);  // RHS data
        जर (!getfunc) {
            माला_दो(", " + off);  // RHS offset
        }
        माला_दो(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    शून्य emitPackedMember(स्थिरांक Ast_गाठ_DType* parentDtypep, स्थिरांक Ast_गाठ_DType* dtypep,
                          स्थिरांक std::string& fieldname, स्थिरांक std::string& offset, तार्किक getfunc,
                          स्थिरांक std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        जर (स्थिरांक स्वतः* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            स्थिरांक std::string index = m_names.get("__Vi");
            माला_दो("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            स्थिरांक std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            स्थिरांक std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            माला_दो("}\n");
        } अन्यथा जर (VN_IS(dtypep, गाठ_UOr_Struct_DType)) {
            स्थिरांक std::string tmp = m_names.get("__Vtmp");
            जर (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                माला_दो("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } अन्यथा {  // Emit `set` func
                स्थिरांक std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                माला_दो(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                माला_दो(fieldname + ".set(" + tmp + ");\n");
            }
        } अन्यथा {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            जर (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } अन्यथा {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    शून्य emitPackedUOrSBody(Ast_गाठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        स्थिरांक AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        साठी (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            जर (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        साठी (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य, सत्य));
            माला_दो(";\n");
        }

        स्थिरांक std::string retArgName = m_names.get("__v");
        स्थिरांक std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        माला_दो(retArgType + " get() const {\n");
        माला_दो(retArgType + " " + retArgName + ";\n");
        जर (VN_IS(sdtypep, StructDType)) {
            साठी (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/सत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/सत्य, retArgName);
        }
        माला_दो("return " + retArgName + ";\n");
        माला_दो("}\n");

        // Emit `set` member function
        माला_दो("void set(const " + retArgType + "& " + retArgName + ") {\n");
        जर (VN_IS(sdtypep, StructDType)) {
            साठी (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/असत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/असत्य, retArgName);
        }

        माला_दो("}\n");

        माला_दो("};\n");
        m_names.reset();
    }
    शून्य emitStructs(स्थिरांक Ast_गाठ_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_गाठ_UOr_Struct_DType*> emitted;
        साठी (स्थिरांक Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिरांक AstTypedef* स्थिरांक tdefp = VN_CAST(nodep, Typedef);
            जर (!tdefp) सुरूठेवा;
            Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), गाठ_UOr_Struct_DType);
            जर (!sdtypep) सुरूठेवा;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            स्थिरांक तार्किक declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            जर (declaredInOtherModule) सुरूठेवा;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    शून्य emitFuncDecls(स्थिरांक Ast_गाठ_Module* modp, तार्किक inClassBody) {
        std::vector<स्थिरांक AstCFunc*> funcsp;

        साठी (स्थिरांक Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstCFunc* स्थिरांक funcp = VN_CAST(nodep, CFunc)) {
                जर (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    सुरूठेवा;
                जर (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    सुरूठेवा;
                जर (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    सुरूठेवा;
                जर (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    सुरूठेवा;
                funcsp.ढकला_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](स्थिरांक Ast_गाठ* ap, स्थिरांक Ast_गाठ* bp) {
            परत ap->name() < bp->name();
        });

        साठी (स्थिरांक AstCFunc* स्थिरांक funcp : funcsp) {
            जर (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    शून्य emitAll(स्थिरांक Ast_गाठ_Module* modp) {
        // Include files required by this AstNodeModule
        जर (स्थिरांक AstClass* स्थिरांक classp = VN_CAST(modp, Class)) {
            साठी (स्थिरांक AstClassExtends* extp = classp->extendsp(); extp;
                 extp = VN_AS(extp->nextp(), ClassExtends)) {
                putns(extp, "#include \""
                                + EmitCUtil::prefixNameProtect(extp->classp()->classOrPackagep())
                                + ".h\"\n");
            }
        }

        // Forward declarations required by this AstNodeModule
        माला_दो("\nclass " + EmitCUtil::symClassName() + ";\n");

        // From `systemc_header
        emitSystemCSection(modp, VSystemCSectionType::HDR);

        emitStructs(modp);

        // Open class body {{{
        माला_दो("\n");
        putns(modp, "class ");
        जर (!VN_IS(modp, Class)) माला_दो("alignas(VL_CACHE_LINE_BYTES) ");
        माला_दो(EmitCUtil::prefixNameProtect(modp));
        जर (स्थिरांक AstClass* स्थिरांक classp = VN_CAST(modp, Class)) {
            माला_दो(" : ");
            जर (classp->extendsp()) {
                तार्किक needComma = असत्य;
                साठी (स्थिरांक AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    जर (needComma) माला_दो(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    माला_दो(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = सत्य;
                }
            } अन्यथा {
                माला_दो("public virtual VlClass");
            }
        } अन्यथा {
            माला_दो(" final");
        }
        माला_दो(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(असत्य);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ सत्य);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        माला_दो("};\n");
        // }}}

        // Emit out of class function declarations
        माला_दो("\n");
        emitFuncDecls(modp, /* inClassBody: */ असत्य);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(स्थिरांक Ast_गाठ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        माला_दो("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        माला_दो("\n");
        ofp()->puts_Int_शीर्ष_Include();
        माला_दो("#include \"verilated.h\"\n");
        जर (modp->is_शीर्ष() && optSystemC()) माला_दो("#include \"verilated_sc.h\"\n");
        जर (v3Global.opt.mtasks()) माला_दो("#include \"verilated_threads.h\"\n");
        जर (v3Global.opt.savable()) माला_दो("#include \"verilated_save.h\"\n");
        जर (v3Global.opt.coverage()) माला_दो("#include \"verilated_cov.h\"\n");
        जर (v3Global.opt.coverage() || v3Global.useCovergroup())
            माला_दो("#include \"verilated_covergroup.h\"\n");
        जर (v3Global.usesTiming()) माला_दो("#include \"verilated_timing.h\"\n");
        जर (v3Global.useRandomizeMethods()) माला_दो("#include \"verilated_random.h\"\n");
        जर (v3Global.usesForce()) माला_दो("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        स्वतः add_to_cuse_set = [&](string s) { cuse_set.घाला(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        जर (स्थिरांक AstClassPackage* स्थिरांक packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        साठी (स्थिरांक string& s : cuse_set) माला_दो(s);
        माला_दो("\n");

        emitAll(modp);

        जर (स्थिरांक AstClassPackage* स्थिरांक packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = पूर्वनिर्धारित;

सार्वजनिक:
    स्थिर शून्य main(स्थिरांक Ast_गाठ_Module* modp) { स्थिरांक EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    साठी (स्थिरांक Ast_गाठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        जर (VN_IS(nodep, Class)) सुरूठेवा;  // Declared with the ClassPackage
        स्थिरांक Ast_गाठ_Module* स्थिरांक modp = VN_AS(nodep, गाठ_Module);
        जर (modp->isConstPool()) सुरूठेवा;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
