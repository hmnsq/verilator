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

    वर्ग Cover_गन्ती_Visitor final : सार्वजनिक VNVisitorConst {
        पूर्णाङ्क m_bins = 0;  // Running total of coverage bins counted so far

        शून्य visit(Ast_गाँठो_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            यदि (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

    सार्वजनिक:
        explicit Cover_गन्ती_Visitor(अचर Ast_गाँठो_Module* modp) {
            iterateConst(const_cast<Ast_गाँठो_Module*>(modp));
        }
        पूर्णाङ्क bins() अचर { फर्काऊ m_bins; }
    };

    शून्य decorateFirst(तार्किक& first, अचर string& str) {
        यदि (first) {
            putsDecoration(nullptr, str);
            first = असत्य;
        }
    }
    शून्य emitCellDecls(अचर Ast_गाँठो_Module* modp) {
        तार्किक first = सत्य;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCell* अचर cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) जारी;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    शून्य emitDesignVarDecls(अचर Ast_गाँठो_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        अचर AstVar* portp = nullptr;
        अचर AstVar* trivp = nullptr;
        अचर AstVar* nontp = nullptr;
        माप_प्रकार nPort = 0;
        माप_प्रकार nTriv = 0;
        माप_प्रकार nNont = 0;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(nodep, Var);
            यदि (!varp) रोक;
            यदि (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_पङ्क्ति_Align(), varp, "Aligned non-field");
            } अन्यथा यदि (varp->isPrimaryIO()) {
                यदि (!portp) portp = varp;
                ++nPort;
            } अन्यथा यदि (varp->isTriviallyConstructible()) {
                यदि (!trivp) trivp = varp;
                ++nTriv;
            } अन्यथा {
                यदि (!nontp) nontp = varp;
                ++nNont;
            }
        }

        अचर स्वतः emitAndStep = [&](अचर AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        यदि (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            माला_दो("union {\nstruct {\n");
            लागि (माप_प्रकार i = 0; i < nPort; ++i) emitAndStep(portp);
            माला_दो("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        यदि (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            यदि (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                लागि (माप_प्रकार i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } अन्यथा {
                अचर माप_प्रकार lim = v3Global.opt.compLimitMembers();
                माप_प्रकार anonL3s = 1;
                माप_प्रकार anonL2s = 1;
                माप_प्रकार anonL1s = 1;
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
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                माप_प्रकार nEmitted = 0;
                लागि (माप_प्रकार l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    यदि (anonL3s != 1) माला_दो("struct {\n");
                    लागि (माप_प्रकार l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        यदि (anonL2s != 1) माला_दो("struct {\n");
                        लागि (माप_प्रकार l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            यदि (anonL1s != 1) माला_दो("struct {\n");
                            लागि (माप_प्रकार l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            यदि (anonL1s != 1) माला_दो("};\n");
                        }
                        यदि (anonL2s != 1) माला_दो("};\n");
                    }
                    यदि (anonL3s != 1) माला_दो("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        यदि (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            यदि (VN_IS(modp, Class)) {
                लागि (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
                फर्काऊ;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            माला_दो("union {\nstruct {\n");
            लागि (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
            माला_दो("} __Vnontrivs;\n};\n");
        }
    }
    शून्य emitInternalVarDecls(अचर Ast_गाँठो_Module* modp) {
        यदि (अचर AstClass* अचर classp = VN_CAST(modp, Class)) {
            यदि (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                माला_दो("VlRNGReseeds __Vm_rng;\n");
            }
        } अन्यथा {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            माला_दो(EmitCUtil::symClassName() + "* vlSymsp;\n");
            माला_दो("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            अचर पूर्णाङ्क coverBins = Cover_गन्ती_Visitor{modp}.bins();
            यदि (coverBins) {
                माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                माला_दो(" __Vcoverage[");
                माला_दो(std::to_string(coverBins));
                माला_दो("]{};\n");
            }
        }
    }
    शून्य emitParamDecls(अचर Ast_गाँठो_Module* modp) {
        तार्किक first = सत्य;
        लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    माला_दो(varp->dtypep()->cType(varp->nameProtect(), असत्य, असत्य));
                    माला_दो(";\n");
                }
            }
        }
    }
    शून्य emitCtorDtorDecls(अचर Ast_गाँठो_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        यदि (VN_IS(modp, Class)) फर्काऊ;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        अचर std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    शून्य emitInternalMethodDecls(अचर Ast_गाँठो_Module* modp) {
        तार्किक first = सत्य;
        अचर string section = "\n// INTERNAL METHODS\n";

        यदि (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            यदि (v3Global.opt.coverage()) {
                माला_दो("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अन्यथा {
            decorateFirst(first, section);
            अचर std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दहरू_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            माला_दो("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        यदि (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        यदि (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            माला_दो("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        यदि (v3Global.opt.savable()) {
            decorateFirst(first, section);
            माला_दो("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            माला_दो("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        यदि (अचर AstClass* अचर classp = VN_CAST(modp, Class)) {
            यदि (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<अचर AstClass*, अचर AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                अचर स्वतः hasEnclosingBackPointer = [](अचर AstClass* covergroupp) {
                    फर्काऊ covergroupp->exists([](अचर AstVar* अचर varp) {
                        अचर AstClassRefDType* अचर refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        फर्काऊ refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* अचर memberClassp, AstVar* अचर varp) {
                        अचर AstClassRefDType* अचर refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        यदि (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                अचर string className = EmitCUtil::prefixNameProtect(classp);
                यदि (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } अन्यथा {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    यदि (classp->hasRandVarsUpdate()) {
                        अचर string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* अचर updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        माला_दो("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    लागि (अचर EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        माला_दो("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    माला_दो("return clonep; }\n");
                }
            }
        }
    }
    शून्य emitEnums(अचर Ast_गाँठो_Module* modp) {
        तार्किक first = सत्य;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अचर AstTypedef* अचर tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) जारी;
            यदि (!tdefp->attrPublic()) जारी;
            अचर AstEnumDType* अचर edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            यदि (!edtypep) जारी;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            यदि (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अन्यथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                लागि (अचर AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    यदि (अचर AstConst* अचर constp = VN_CAST(itemp->valuep(), Const)) {
                        यदि (constp->अङ्क().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            जारी;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    माला_दो(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अङ्क().emitC());
                    यदि (VN_IS(itemp->nextp(), EnumItem)) माला_दो(",");
                    माला_दो("\n");
                }
                माला_दो("};\n");
            }
        }
    }
    शून्य emitStructDecl(अचर Ast_गाँठो_Module* modp, Ast_गाँठो_UOr_Struct_DType* sdtypep,
                        std::set<Ast_गाँठो_UOr_Struct_DType*>& emitted) {
        यदि (emitted.गन्ती(sdtypep) > 0) फर्काऊ;
        emitted.घुसाउ(sdtypep);
        लागि (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाँठो_UOr_Struct_DType* अचर subp = itemp->getChildStructp();
            यदि (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                यदि (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    माला_दो("\n");
                }
            }
        }
        यदि (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } अन्यथा {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    गणना वर्ग AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    पूर्णाङ्क get_गाँठो_Attribute(अचर AstMemberDType* itemp, AttributeType type) {
        अचर तार्किक isArrayType = itemp->dtypep()->isNonPackedArray();
        विकल्प (type) {
        पक्ष AttributeType::WIDTH: {
            यदि (isArrayType) {
                // For arrays, get innermost element width
                अचर Ast_गाँठो_DType* dtype = itemp->dtypep();
                जबसम्म (dtype->subDTypep()) dtype = dtype->subDTypep();
                फर्काऊ dtype->width();
            }
            फर्काऊ itemp->width();
        }
        पक्ष AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            फर्काऊ isArrayType ? itemp->dtypep()->dimensions(सत्य).second : 0;
        }
        स्वभाव: {
            फर्काऊ 0;
        }
        }
    }
    ढाँचा <AttributeType T>
    शून्य emitMemberVector(अचर Ast_गाँठो_UOr_Struct_DType* sdtypep) {
        माला_दो("return {");
        तार्किक needComma = असत्य;
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (!itemp->isConstrainedRand()) जारी;
            // Comma handling: add before element except first
            यदि (needComma) माला_दो(",\n");
            putns(itemp, std::to_string(get_गाँठो_Attribute(itemp, T)));
            needComma = सत्य;
        }
        माला_दो("};\n}\n");
    }
    शून्य emitUnpackedUOrSBody(Ast_गाँठो_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
        यदि (sdtypep->isConstrainedRand()) {
            तार्किक needComma = असत्य;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            माला_दो("return {");
            लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) जारी;
                यदि (needComma) माला_दो(",\n");
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
            लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) जारी;
                यदि (needComma) माला_दो(",\n");
                putns(itemp, itemp->dtypep()->cType("", असत्य, असत्य));
                needComma = सत्य;
            }
            माला_दो(">{};\n}\n");

            needComma = असत्य;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            माला_दो("return std::tie(");
            लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) जारी;
                यदि (needComma) माला_दो(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = सत्य;
            }
            माला_दो(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return ");
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (itemp != sdtypep->membersp()) माला_दो("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        माला_दो(";\n");
        माला_दो("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    शून्य emitVlAssign(अचर Ast_गाँठो_DType* अचर lhstype, अचर Ast_गाँठो_DType* rhstype,
                      अचर std::string& off, अचर std::string& lhsdata,
                      अचर std::string& rhsdata, तार्किक getfunc) {
        माला_दो(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        माला_दो(lhstype->charIQWN());
        माला_दो(rhstype->charIQWN());
        माला_दो("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        यदि (getfunc) {
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
        यदि (!getfunc) {
            माला_दो(", " + off);  // RHS offset
        }
        माला_दो(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    शून्य emitPackedMember(अचर Ast_गाँठो_DType* parentDtypep, अचर Ast_गाँठो_DType* dtypep,
                          अचर std::string& fieldname, अचर std::string& offset, तार्किक getfunc,
                          अचर std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        यदि (अचर स्वतः* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            अचर std::string index = m_names.get("__Vi");
            माला_दो("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            अचर std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            अचर std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            माला_दो("}\n");
        } अन्यथा यदि (VN_IS(dtypep, गाँठो_UOr_Struct_DType)) {
            अचर std::string tmp = m_names.get("__Vtmp");
            यदि (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                माला_दो("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } अन्यथा {  // Emit `set` func
                अचर std::string tmptype = AstCDType::typeToHold(dtypep->width());
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
            यदि (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } अन्यथा {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    शून्य emitPackedUOrSBody(Ast_गाँठो_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        अचर AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        लागि (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            यदि (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        लागि (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य, सत्य));
            माला_दो(";\n");
        }

        अचर std::string retArgName = m_names.get("__v");
        अचर std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        माला_दो(retArgType + " get() const {\n");
        माला_दो(retArgType + " " + retArgName + ";\n");
        यदि (VN_IS(sdtypep, StructDType)) {
            लागि (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
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
        यदि (VN_IS(sdtypep, StructDType)) {
            लागि (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
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
    शून्य emitStructs(अचर Ast_गाँठो_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_गाँठो_UOr_Struct_DType*> emitted;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अचर AstTypedef* अचर tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) जारी;
            Ast_गाँठो_UOr_Struct_DType* अचर sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), गाँठो_UOr_Struct_DType);
            यदि (!sdtypep) जारी;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            अचर तार्किक declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            यदि (declaredInOtherModule) जारी;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    शून्य emitFuncDecls(अचर Ast_गाँठो_Module* modp, तार्किक inClassBody) {
        std::vector<अचर AstCFunc*> funcsp;

        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCFunc* अचर funcp = VN_CAST(nodep, CFunc)) {
                यदि (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    जारी;
                यदि (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    जारी;
                यदि (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    जारी;
                यदि (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    जारी;
                funcsp.थप_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](अचर Ast_गाँठो* ap, अचर Ast_गाँठो* bp) {
            फर्काऊ ap->name() < bp->name();
        });

        लागि (अचर AstCFunc* अचर funcp : funcsp) {
            यदि (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    शून्य emitAll(अचर Ast_गाँठो_Module* modp) {
        // Include files required by this AstNodeModule
        यदि (अचर AstClass* अचर classp = VN_CAST(modp, Class)) {
            लागि (अचर AstClassExtends* extp = classp->extendsp(); extp;
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
        यदि (!VN_IS(modp, Class)) माला_दो("alignas(VL_CACHE_LINE_BYTES) ");
        माला_दो(EmitCUtil::prefixNameProtect(modp));
        यदि (अचर AstClass* अचर classp = VN_CAST(modp, Class)) {
            माला_दो(" : ");
            यदि (classp->extendsp()) {
                तार्किक needComma = असत्य;
                लागि (अचर AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    यदि (needComma) माला_दो(", ");
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
    शून्य visit(Ast_गाँठो* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(अचर Ast_गाँठो_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        माला_दो("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        माला_दो("\n");
        ofp()->puts_Int_शीर्ष_Include();
        माला_दो("#include \"verilated.h\"\n");
        यदि (modp->is_शीर्ष() && optSystemC()) माला_दो("#include \"verilated_sc.h\"\n");
        यदि (v3Global.opt.mtasks()) माला_दो("#include \"verilated_threads.h\"\n");
        यदि (v3Global.opt.savable()) माला_दो("#include \"verilated_save.h\"\n");
        यदि (v3Global.opt.coverage()) माला_दो("#include \"verilated_cov.h\"\n");
        यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
            माला_दो("#include \"verilated_covergroup.h\"\n");
        यदि (v3Global.usesTiming()) माला_दो("#include \"verilated_timing.h\"\n");
        यदि (v3Global.useRandomizeMethods()) माला_दो("#include \"verilated_random.h\"\n");
        यदि (v3Global.usesForce()) माला_दो("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        स्वतः add_to_cuse_set = [&](string s) { cuse_set.घुसाउ(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        यदि (अचर AstClassPackage* अचर packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        लागि (अचर string& s : cuse_set) माला_दो(s);
        माला_दो("\n");

        emitAll(modp);

        यदि (अचर AstClassPackage* अचर packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य main(अचर Ast_गाँठो_Module* modp) { अचर EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    लागि (अचर Ast_गाँठो* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        यदि (VN_IS(nodep, Class)) जारी;  // Declared with the ClassPackage
        अचर Ast_गाँठो_Module* अचर modp = VN_AS(nodep, गाँठो_Module);
        यदि (modp->isConstPool()) जारी;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
