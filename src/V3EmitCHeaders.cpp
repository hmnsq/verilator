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
        पूर्णाङ्क m_bins = 0;  // Running total of coverage bins counted so far

        शून्य visit(Ast_गाठ_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            यदि (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    सार्वजनिक:
        explicit Cover_मोजणी_Visitor(ध्रुव Ast_गाठ_Module* modp) {
            iterateConst(const_cast<Ast_गाठ_Module*>(modp));
        }
        पूर्णाङ्क bins() ध्रुव { प्रत्यागम m_bins; }
    };

    शून्य decorateFirst(तार्किक& first, ध्रुव string& str) {
        यदि (first) {
            putsDecoration(nullptr, str);
            first = असत्य;
        }
    }
    शून्य emitCellDecls(ध्रुव Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    शून्य emitDesignVarDecls(ध्रुव Ast_गाठ_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ध्रुव AstVar* portp = nullptr;
        ध्रुव AstVar* trivp = nullptr;
        ध्रुव AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (!varp) भङ्ग;
            यदि (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_ओळ_Align(), varp, "Aligned non-field");
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

        ध्रुव auto emitAndStep = [&](ध्रुव AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        यदि (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            प्रति (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        यदि (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            यदि (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                प्रति (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } अन्यथा {
                ध्रुव size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
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
                size_t nEmitted = 0;
                प्रति (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    यदि (anonL3s != 1) puts("struct {\n");
                    प्रति (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        यदि (anonL2s != 1) puts("struct {\n");
                        प्रति (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            यदि (anonL1s != 1) puts("struct {\n");
                            प्रति (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            यदि (anonL1s != 1) puts("};\n");
                        }
                        यदि (anonL2s != 1) puts("};\n");
                    }
                    यदि (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        यदि (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            यदि (VN_IS(modp, Class)) {
                प्रति (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                प्रत्यागम;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            प्रति (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    शून्य emitInternalVarDecls(ध्रुव Ast_गाठ_Module* modp) {
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            यदि (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } अन्यथा {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ध्रुव पूर्णाङ्क coverBins = Cover_मोजणी_Visitor{modp}.bins();
            यदि (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    शून्य emitParamDecls(ध्रुव Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        प्रति (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), असत्य, असत्य));
                    puts(";\n");
                }
            }
        }
    }
    शून्य emitCtorDtorDecls(ध्रुव Ast_गाठ_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        यदि (VN_IS(modp, Class)) प्रत्यागम;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ध्रुव std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    शून्य emitInternalMethodDecls(ध्रुव Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        ध्रुव string section = "\n// INTERNAL METHODS\n";

        यदि (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            यदि (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } अन्यथा {
            decorateFirst(first, section);
            ध्रुव std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्द_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        यदि (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        यदि (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        यदि (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            यदि (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ध्रुव AstClass*, ध्रुव AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ध्रुव auto hasEnclosingBackPointer = [](ध्रुव AstClass* covergroupp) {
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
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ध्रुव string className = EmitCUtil::prefixNameProtect(classp);
                यदि (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
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
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    प्रति (ध्रुव EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    शून्य emitEnums(ध्रुव Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) अनुवर्त;
            यदि (!tdefp->attrPublic()) अनुवर्त;
            ध्रुव AstEnumDType* ध्रुव edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            यदि (!edtypep) अनुवर्त;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            यदि (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } अन्यथा {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                प्रति (ध्रुव AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(itemp->valuep(), Const)) {
                        यदि (constp->अंक().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            अनुवर्त;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->अंक().emitC());
                    यदि (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    शून्य emitStructDecl(ध्रुव Ast_गाठ_Module* modp, Ast_गाठ_UOr_Struct_DType* sdtypep,
                        std::set<Ast_गाठ_UOr_Struct_DType*>& emitted) {
        यदि (emitted.मोजणी(sdtypep) > 0) प्रत्यागम;
        emitted.घाला(sdtypep);
        प्रति (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाठ_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp();
            यदि (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                यदि (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
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
    पूर्णाङ्क get_गाठ_Attribute(ध्रुव AstMemberDType* itemp, AttributeType type) {
        ध्रुव तार्किक isArrayType = itemp->dtypep()->isNonPackedArray();
        विकल्प (type) {
        पक्ष AttributeType::WIDTH: {
            यदि (isArrayType) {
                // For arrays, get innermost element width
                ध्रुव Ast_गाठ_DType* dtype = itemp->dtypep();
                यावत् (dtype->subDTypep()) dtype = dtype->subDTypep();
                प्रत्यागम dtype->width();
            }
            प्रत्यागम itemp->width();
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
    template <AttributeType T>
    शून्य emitMemberVector(ध्रुव Ast_गाठ_UOr_Struct_DType* sdtypep) {
        puts("return {");
        तार्किक needComma = असत्य;
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (!itemp->isConstrainedRand()) अनुवर्त;
            // Comma handling: add before element except first
            यदि (needComma) puts(",\n");
            putns(itemp, std::to_string(get_गाठ_Attribute(itemp, T)));
            needComma = सत्य;
        }
        puts("};\n}\n");
    }
    शून्य emitUnpackedUOrSBody(Ast_गाठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य));
            puts(";\n");
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
            puts("return {");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = सत्य;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = असत्य;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", असत्य, असत्य));
                needComma = सत्य;
            }
            puts(">{};\n}\n");

            needComma = असत्य;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                यदि (!itemp->isConstrainedRand()) अनुवर्त;
                यदि (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = सत्य;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    शून्य emitVlAssign(ध्रुव Ast_गाठ_DType* ध्रुव lhstype, ध्रुव Ast_गाठ_DType* rhstype,
                      ध्रुव std::string& off, ध्रुव std::string& lhsdata,
                      ध्रुव std::string& rhsdata, तार्किक getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        यदि (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } अन्यथा {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        यदि (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    शून्य emitPackedMember(ध्रुव Ast_गाठ_DType* parentDtypep, ध्रुव Ast_गाठ_DType* dtypep,
                          ध्रुव std::string& fieldname, ध्रुव std::string& offset, तार्किक getfunc,
                          ध्रुव std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        यदि (ध्रुव auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ध्रुव std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ध्रुव std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ध्रुव std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } अन्यथा यदि (VN_IS(dtypep, गाठ_UOr_Struct_DType)) {
            ध्रुव std::string tmp = m_names.get("__Vtmp");
            यदि (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } अन्यथा {  // Emit `set` func
                ध्रुव std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
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
    शून्य emitPackedUOrSBody(Ast_गाठ_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ध्रुव AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        प्रति (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            यदि (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        प्रति (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), असत्य, असत्य, सत्य));
            puts(";\n");
        }

        ध्रुव std::string retArgName = m_names.get("__v");
        ध्रुव std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        यदि (VN_IS(sdtypep, StructDType)) {
            प्रति (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/सत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/सत्य, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        यदि (VN_IS(sdtypep, StructDType)) {
            प्रति (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/असत्य, retArgName);
            }
        } अन्यथा {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/असत्य, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    शून्य emitStructs(ध्रुव Ast_गाठ_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_गाठ_UOr_Struct_DType*> emitted;
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(nodep, Typedef);
            यदि (!tdefp) अनुवर्त;
            Ast_गाठ_UOr_Struct_DType* ध्रुव sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), गाठ_UOr_Struct_DType);
            यदि (!sdtypep) अनुवर्त;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ध्रुव तार्किक declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            यदि (declaredInOtherModule) अनुवर्त;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    शून्य emitFuncDecls(ध्रुव Ast_गाठ_Module* modp, तार्किक inClassBody) {
        std::vector<ध्रुव AstCFunc*> funcsp;

        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
                यदि (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    अनुवर्त;
                यदि (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    अनुवर्त;
                यदि (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    अनुवर्त;
                यदि (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    अनुवर्त;
                funcsp.ढकला_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ध्रुव Ast_गाठ* ap, ध्रुव Ast_गाठ* bp) {
            प्रत्यागम ap->name() < bp->name();
        });

        प्रति (ध्रुव AstCFunc* ध्रुव funcp : funcsp) {
            यदि (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    शून्य emitAll(ध्रुव Ast_गाठ_Module* modp) {
        // Include files required by this AstNodeModule
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            प्रति (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
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
        यदि (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
            puts(" : ");
            यदि (classp->extendsp()) {
                तार्किक needComma = असत्य;
                प्रति (ध्रुव AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    यदि (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = सत्य;
                }
            } अन्यथा {
                puts("public virtual VlClass");
            }
        } अन्यथा {
            puts(" final");
        }
        puts(" {\n");
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
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ असत्य);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ध्रुव Ast_गाठ_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_शीर्ष_Include();
        puts("#include \"verilated.h\"\n");
        यदि (modp->is_शीर्ष() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        यदि (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        यदि (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        यदि (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        यदि (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        यदि (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        यदि (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.घाला(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        प्रति (ध्रुव string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य main(ध्रुव Ast_गाठ_Module* modp) { ध्रुव EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    प्रति (ध्रुव Ast_गाठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        यदि (VN_IS(nodep, Class)) अनुवर्त;  // Declared with the ClassPackage
        ध्रुव Ast_गाठ_Module* ध्रुव modp = VN_AS(nodep, गाठ_Module);
        यदि (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
