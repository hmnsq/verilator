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
#समावेश "V3EmitCBase.h"
#समावेश "V3File.h"
#समावेश "V3MemberMap.h"
#समावेश "V3UniqueNames.h"

#समावेश <algorithm>
#समावेश <cstdint>
#समावेश <set>
#समावेश <string>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

श्रेणी EmitCHeader final : खुला EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    श्रेणी Cover_गिनती_Visitor final : खुला VNVisitorConst {
        पूर्णांक m_bins = 0;  // Running total of coverage bins counted so far

        व्योम visit(Ast_गाँठ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            अगर (!nodep->तथ्य_Decl_Nullp()) m_bins += nodep->आकार();
        }
        व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    खुला:
        explicit Cover_गिनती_Visitor(स्थिर Ast_गाँठ_Module* modp) {
            iterateConst(const_cast<Ast_गाँठ_Module*>(modp));
        }
        पूर्णांक bins() स्थिर { वापस m_bins; }
    };

    व्योम decorateFirst(bool& first, स्थिर string& str) {
        अगर (first) {
            putsDecoration(nullptr, str);
            first = false;
        }
    }
    व्योम emitCellDecls(स्थिर Ast_गाँठ_Module* modp) {
        bool first = true;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstCell* स्थिर cellp = VN_CAST(nodep, Cell)) {
                अगर (cellp->modp()->isConstPool()) जारी;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    व्योम emitDesignVarDecls(स्थिर Ast_गाँठ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        स्थिर AstVar* portp = nullptr;
        स्थिर AstVar* trivp = nullptr;
        स्थिर AstVar* nontp = nullptr;
        माप_प्रकार nPort = 0;
        माप_प्रकार nTriv = 0;
        माप_प्रकार nNont = 0;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
            अगर (!varp) अवरोध;
            अगर (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_पंक्ति_Align(), varp, "Aligned non-field");
            } अन्यथा अगर (varp->isPrimaryIO()) {
                अगर (!portp) portp = varp;
                ++nPort;
            } अन्यथा अगर (varp->isTriviallyConstructible()) {
                अगर (!trivp) trivp = varp;
                ++nTriv;
            } अन्यथा {
                अगर (!nontp) nontp = varp;
                ++nNont;
            }
        }

        स्थिर स्वतः emitAndStep = [&](स्थिर AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        अगर (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            माला_दो("union {\nstruct {\n");
            क्रम (माप_प्रकार i = 0; i < nPort; ++i) emitAndStep(portp);
            माला_दो("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        अगर (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            अगर (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                क्रम (माप_प्रकार i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } अन्यथा {
                स्थिर माप_प्रकार lim = v3Global.opt.compLimitMembers();
                माप_प्रकार anonL3s = 1;
                माप_प्रकार anonL2s = 1;
                माप_प्रकार anonL1s = 1;
                अगर (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } अन्यथा अगर (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } अन्यथा अगर (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                अगर (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                माप_प्रकार nEmitted = 0;
                क्रम (माप_प्रकार l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    अगर (anonL3s != 1) माला_दो("struct {\n");
                    क्रम (माप_प्रकार l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        अगर (anonL2s != 1) माला_दो("struct {\n");
                        क्रम (माप_प्रकार l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            अगर (anonL1s != 1) माला_दो("struct {\n");
                            क्रम (माप_प्रकार l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            अगर (anonL1s != 1) माला_दो("};\n");
                        }
                        अगर (anonL2s != 1) माला_दो("};\n");
                    }
                    अगर (anonL3s != 1) माला_दो("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        अगर (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            अगर (VN_IS(modp, Class)) {
                क्रम (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
                वापस;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            माला_दो("union {\nstruct {\n");
            क्रम (माप_प्रकार i = 0; i < nNont; ++i) emitAndStep(nontp);
            माला_दो("} __Vnontrivs;\n};\n");
        }
    }
    व्योम emitInternalVarDecls(स्थिर Ast_गाँठ_Module* modp) {
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(modp, Class)) {
            अगर (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                माला_दो("VlRNGReseeds __Vm_rng;\n");
            }
        } अन्यथा {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            माला_दो(EmitCUtil::symClassName() + "* vlSymsp;\n");
            माला_दो("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            स्थिर पूर्णांक coverBins = Cover_गिनती_Visitor{modp}.bins();
            अगर (coverBins) {
                माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                माला_दो(" __Vcoverage[");
                माला_दो(std::to_string(coverBins));
                माला_दो("]{};\n");
            }
        }
    }
    व्योम emitParamDecls(स्थिर Ast_गाँठ_Module* modp) {
        bool first = true;
        क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                अगर (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    माला_दो(varp->dtypep()->cType(varp->nameProtect(), false, false));
                    माला_दो(";\n");
                }
            }
        }
    }
    व्योम emitCtorDtorDecls(स्थिर Ast_गाँठ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        अगर (VN_IS(modp, Class)) वापस;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        स्थिर std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    व्योम emitInternalMethodDecls(स्थिर Ast_गाँठ_Module* modp) {
        bool first = true;
        स्थिर string section = "\n// INTERNAL METHODS\n";

        अगर (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            अगर (v3Global.opt.coverage()) {
                माला_दो("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अन्यथा {
            decorateFirst(first, section);
            स्थिर std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दगण_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            माला_दो("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        अगर (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        अगर (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            माला_दो("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        अगर (v3Global.opt.savable()) {
            decorateFirst(first, section);
            माला_दो("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            माला_दो("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(modp, Class)) {
            अगर (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<स्थिर AstClass*, स्थिर AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                स्थिर स्वतः hasEnclosingBackPointer = [](स्थिर AstClass* covergroupp) {
                    वापस covergroupp->exists([](स्थिर AstVar* स्थिर varp) {
                        स्थिर AstClassRefDType* स्थिर refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        वापस refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* स्थिर memberClassp, AstVar* स्थिर varp) {
                        स्थिर AstClassRefDType* स्थिर refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        अगर (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                स्थिर string className = EmitCUtil::prefixNameProtect(classp);
                अगर (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } अन्यथा {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    अगर (classp->hasRandVarsUpdate()) {
                        स्थिर string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* स्थिर updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        माला_दो("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    क्रम (स्थिर EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        माला_दो("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    माला_दो("return clonep; }\n");
                }
            }
        }
    }
    व्योम emitEnums(स्थिर Ast_गाँठ_Module* modp) {
        bool first = true;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिर AstTypedef* स्थिर tdefp = VN_CAST(nodep, Typedef);
            अगर (!tdefp) जारी;
            अगर (!tdefp->attrPublic()) जारी;
            स्थिर AstEnumDType* स्थिर edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            अगर (!edtypep) जारी;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            अगर (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अन्यथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                क्रम (स्थिर AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    अगर (स्थिर AstConst* स्थिर constp = VN_CAST(itemp->valuep(), Const)) {
                        अगर (constp->अंक().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            जारी;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    माला_दो(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अंक().emitC());
                    अगर (VN_IS(itemp->nextp(), EnumItem)) माला_दो(",");
                    माला_दो("\n");
                }
                माला_दो("};\n");
            }
        }
    }
    व्योम emitStructDecl(स्थिर Ast_गाँठ_Module* modp, Ast_गाँठ_UOr_Struct_DType* sdtypep,
                        std::set<Ast_गाँठ_UOr_Struct_DType*>& emitted) {
        अगर (emitted.गिनती(sdtypep) > 0) वापस;
        emitted.जोड़ो(sdtypep);
        क्रम (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाँठ_UOr_Struct_DType* स्थिर subp = itemp->getChildStructp();
            अगर (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                अगर (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    माला_दो("\n");
                }
            }
        }
        अगर (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } अन्यथा {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    क्रमागत श्रेणी AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    पूर्णांक get_गाँठ_Attribute(स्थिर AstMemberDType* itemp, AttributeType type) {
        स्थिर bool isArrayType = itemp->dtypep()->isNonPackedArray();
        चयन (type) {
        हाल AttributeType::WIDTH: {
            अगर (isArrayType) {
                // For arrays, get innermost element width
                स्थिर Ast_गाँठ_DType* dtype = itemp->dtypep();
                जबतक (dtype->subDTypep()) dtype = dtype->subDTypep();
                वापस dtype->width();
            }
            वापस itemp->width();
        }
        हाल AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            वापस isArrayType ? itemp->dtypep()->dimensions(true).second : 0;
        }
        शेष: {
            वापस 0;
        }
        }
    }
    ढाँचा <AttributeType T>
    व्योम emitMemberVector(स्थिर Ast_गाँठ_UOr_Struct_DType* sdtypep) {
        माला_दो("return {");
        bool needComma = false;
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अगर (!itemp->isConstrainedRand()) जारी;
            // Comma handling: add before element except first
            अगर (needComma) माला_दो(",\n");
            putns(itemp, std::to_string(get_गाँठ_Attribute(itemp, T)));
            needComma = true;
        }
        माला_दो("};\n}\n");
    }
    व्योम emitUnpackedUOrSBody(Ast_गाँठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), false, false));
            माला_दो(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        अगर (sdtypep->isConstrainedRand()) {
            bool needComma = false;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            माला_दो("return {");
            क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                अगर (!itemp->isConstrainedRand()) जारी;
                अगर (needComma) माला_दो(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = true;
            }
            माला_दो("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = false;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            माला_दो("return std::index_sequence_for<");
            क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                अगर (!itemp->isConstrainedRand()) जारी;
                अगर (needComma) माला_दो(",\n");
                putns(itemp, itemp->dtypep()->cType("", false, false));
                needComma = true;
            }
            माला_दो(">{};\n}\n");

            needComma = false;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            माला_दो("return std::tie(");
            क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                अगर (!itemp->isConstrainedRand()) जारी;
                अगर (needComma) माला_दो(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = true;
            }
            माला_दो(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return ");
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अगर (itemp != sdtypep->membersp()) माला_दो("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        माला_दो(";\n");
        माला_दो("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        माला_दो("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    व्योम emitVlAssign(स्थिर Ast_गाँठ_DType* स्थिर lhstype, स्थिर Ast_गाँठ_DType* rhstype,
                      स्थिर std::string& off, स्थिर std::string& lhsdata,
                      स्थिर std::string& rhsdata, bool getfunc) {
        माला_दो(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        माला_दो(lhstype->charIQWN());
        माला_दो(rhstype->charIQWN());
        माला_दो("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        अगर (getfunc) {
            माला_दो(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            माला_दो(off + ", ");  // LHS offset
        } अन्यथा {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            माला_दो(std::to_string(lhstype->width_योग_Bytes() * 8) + ", ");
        }
        माला_दो(lhsdata + ", ");  // LHS data
        माला_दो(rhsdata);  // RHS data
        अगर (!getfunc) {
            माला_दो(", " + off);  // RHS offset
        }
        माला_दो(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    व्योम emitPackedMember(स्थिर Ast_गाँठ_DType* parentDtypep, स्थिर Ast_गाँठ_DType* dtypep,
                          स्थिर std::string& fieldname, स्थिर std::string& offset, bool getfunc,
                          स्थिर std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        अगर (स्थिर स्वतः* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            स्थिर std::string सूचकांक_2 = m_names.लो("__Vi");
            माला_दो("for (int " + सूचकांक_2 + " = 0; " + सूचकांक_2 + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + सूचकांक_2 + ") {\n");

            स्थिर std::string offsetInLoop
                = offset + " + " + सूचकांक_2 + " * " + std::to_string(adtypep->subDTypep()->width());
            स्थिर std::string newName = fieldname + "[" + सूचकांक_2 + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            माला_दो("}\n");
        } अन्यथा अगर (VN_IS(dtypep, गाँठ_UOr_Struct_DType)) {
            स्थिर std::string tmp = m_names.लो("__Vtmp");
            अगर (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                माला_दो("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } अन्यथा {  // Emit `set` func
                स्थिर std::string tmptype = AstCDType::typeToHold(dtypep->width());
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
            अगर (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } अन्यथा {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    व्योम emitPackedUOrSBody(Ast_गाँठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        माला_दो(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        स्थिर AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        क्रम (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            अगर (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        क्रम (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), false, false, true));
            माला_दो(";\n");
        }

        स्थिर std::string retArgName = m_names.लो("__v");
        स्थिर std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        माला_दो(retArgType + " get() const {\n");
        माला_दो(retArgType + " " + retArgName + ";\n");
        अगर (VN_IS(sdtypep, StructDType)) {
            क्रम (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/true, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/true, retArgName);
        }
        माला_दो("return " + retArgName + ";\n");
        माला_दो("}\n");

        // Emit `set` member function
        माला_दो("void set(const " + retArgType + "& " + retArgName + ") {\n");
        अगर (VN_IS(sdtypep, StructDType)) {
            क्रम (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/false, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/false, retArgName);
        }

        माला_दो("}\n");

        माला_दो("};\n");
        m_names.reset();
    }
    व्योम emitStructs(स्थिर Ast_गाँठ_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_गाँठ_UOr_Struct_DType*> emitted;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिर AstTypedef* स्थिर tdefp = VN_CAST(nodep, Typedef);
            अगर (!tdefp) जारी;
            Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), गाँठ_UOr_Struct_DType);
            अगर (!sdtypep) जारी;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            स्थिर bool declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            अगर (declaredInOtherModule) जारी;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    व्योम emitFuncDecls(स्थिर Ast_गाँठ_Module* modp, bool inClassBody) {
        std::vector<स्थिर AstCFunc*> funcsp;

        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstCFunc* स्थिर funcp = VN_CAST(nodep, CFunc)) {
                अगर (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    जारी;
                अगर (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    जारी;
                अगर (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    जारी;
                अगर (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    जारी;
                funcsp.धकेलो_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](स्थिर Ast_गाँठ* ap, स्थिर Ast_गाँठ* bp) {
            वापस ap->name() < bp->name();
        });

        क्रम (स्थिर AstCFunc* स्थिर funcp : funcsp) {
            अगर (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    व्योम emitAll(स्थिर Ast_गाँठ_Module* modp) {
        // Include files required by this AstNodeModule
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(modp, Class)) {
            क्रम (स्थिर AstClassExtends* extp = classp->extendsp(); extp;
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
        अगर (!VN_IS(modp, Class)) माला_दो("alignas(VL_CACHE_LINE_BYTES) ");
        माला_दो(EmitCUtil::prefixNameProtect(modp));
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(modp, Class)) {
            माला_दो(" : ");
            अगर (classp->extendsp()) {
                bool needComma = false;
                क्रम (स्थिर AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    अगर (needComma) माला_दो(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    माला_दो(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = true;
                }
            } अन्यथा {
                माला_दो("public virtual VlClass");
            }
        } अन्यथा {
            माला_दो(" final");
        }
        माला_दो(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(false);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ true);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        माला_दो("};\n");
        // }}}

        // Emit out of class function declarations
        माला_दो("\n");
        emitFuncDecls(modp, /* inClassBody: */ false);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(स्थिर Ast_गाँठ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        माला_दो("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        माला_दो("\n");
        ofp()->puts_Int_शीर्ष_Include();
        माला_दो("#include \"verilated.h\"\n");
        अगर (modp->is_शीर्ष() && optSystemC()) माला_दो("#include \"verilated_sc.h\"\n");
        अगर (v3Global.opt.mtasks()) माला_दो("#include \"verilated_threads.h\"\n");
        अगर (v3Global.opt.savable()) माला_दो("#include \"verilated_save.h\"\n");
        अगर (v3Global.opt.coverage()) माला_दो("#include \"verilated_cov.h\"\n");
        अगर (v3Global.opt.coverage() || v3Global.useCovergroup())
            माला_दो("#include \"verilated_covergroup.h\"\n");
        अगर (v3Global.usesTiming()) माला_दो("#include \"verilated_timing.h\"\n");
        अगर (v3Global.useRandomizeMethods()) माला_दो("#include \"verilated_random.h\"\n");
        अगर (v3Global.usesForce()) माला_दो("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        स्वतः add_to_cuse_set = [&](string s) { cuse_set.जोड़ो(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        अगर (स्थिर AstClassPackage* स्थिर packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        क्रम (स्थिर string& s : cuse_set) माला_दो(s);
        माला_दो("\n");

        emitAll(modp);

        अगर (स्थिर AstClassPackage* स्थिर packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = शेष;

खुला:
    अटल व्योम मुख्य(स्थिर Ast_गाँठ_Module* modp) { स्थिर EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

व्योम V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    क्रम (स्थिर Ast_गाँठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        अगर (VN_IS(nodep, Class)) जारी;  // Declared with the ClassPackage
        स्थिर Ast_गाँठ_Module* स्थिर modp = VN_AS(nodep, गाँठ_Module);
        अगर (modp->isConstPool()) जारी;  // Emitted by V3EmitCConstPool
        EmitCHeader::मुख्य(modp);
    }
}
