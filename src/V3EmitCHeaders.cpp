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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cstdint>
#அந்தர்பாவ <set>
#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

வகுப்பு EmitCHeader final : பொது EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    வகுப்பு Cover_மொத்தம்_Visitor final : பொது VNVisitorConst {
        முழு m_bins = 0;  // Running total of coverage bins counted so far

        வெறுமை visit(Ast_முனை_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            எனில் (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

    பொது:
        explicit Cover_மொத்தம்_Visitor(மாறிலி Ast_முனை_Module* modp) {
            iterateConst(const_cast<Ast_முனை_Module*>(modp));
        }
        முழு bins() மாறிலி { திரும்பு m_bins; }
    };

    வெறுமை decorateFirst(தருக்க& first, மாறிலி string& str) {
        எனில் (first) {
            putsDecoration(nullptr, str);
            first = பொய்;
        }
    }
    வெறுமை emitCellDecls(மாறிலி Ast_முனை_Module* modp) {
        தருக்க first = உண்மை;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(nodep, Cell)) {
                எனில் (cellp->modp()->isConstPool()) தொடர்;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    வெறுமை emitDesignVarDecls(மாறிலி Ast_முனை_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        மாறிலி AstVar* portp = nullptr;
        மாறிலி AstVar* trivp = nullptr;
        மாறிலி AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
            எனில் (!varp) நிறுத்து;
            எனில் (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_வரி_Align(), varp, "Aligned non-field");
            } இல்லையெனில் எனில் (varp->isPrimaryIO()) {
                எனில் (!portp) portp = varp;
                ++nPort;
            } இல்லையெனில் எனில் (varp->isTriviallyConstructible()) {
                எனில் (!trivp) trivp = varp;
                ++nTriv;
            } இல்லையெனில் {
                எனில் (!nontp) nontp = varp;
                ++nNont;
            }
        }

        மாறிலி auto emitAndStep = [&](மாறிலி AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        எனில் (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            க்கு (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        எனில் (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            எனில் (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                க்கு (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } இல்லையெனில் {
                மாறிலி size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                எனில் (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } இல்லையெனில் எனில் (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } இல்லையெனில் எனில் (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                எனில் (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                க்கு (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    எனில் (anonL3s != 1) puts("struct {\n");
                    க்கு (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        எனில் (anonL2s != 1) puts("struct {\n");
                        க்கு (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            எனில் (anonL1s != 1) puts("struct {\n");
                            க்கு (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            எனில் (anonL1s != 1) puts("};\n");
                        }
                        எனில் (anonL2s != 1) puts("};\n");
                    }
                    எனில் (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        எனில் (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            எனில் (VN_IS(modp, Class)) {
                க்கு (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                திரும்பு;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            க்கு (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    வெறுமை emitInternalVarDecls(மாறிலி Ast_முனை_Module* modp) {
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(modp, Class)) {
            எனில் (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } இல்லையெனில் {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            மாறிலி முழு coverBins = Cover_மொத்தம்_Visitor{modp}.bins();
            எனில் (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    வெறுமை emitParamDecls(மாறிலி Ast_முனை_Module* modp) {
        தருக்க first = உண்மை;
        க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), பொய், பொய்));
                    puts(";\n");
                }
            }
        }
    }
    வெறுமை emitCtorDtorDecls(மாறிலி Ast_முனை_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        எனில் (VN_IS(modp, Class)) திரும்பு;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        மாறிலி std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    வெறுமை emitInternalMethodDecls(மாறிலி Ast_முனை_Module* modp) {
        தருக்க first = உண்மை;
        மாறிலி string section = "\n// INTERNAL METHODS\n";

        எனில் (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            எனில் (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } இல்லையெனில் {
            decorateFirst(first, section);
            மாறிலி std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_சொற்கள்_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        எனில் (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        எனில் (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        எனில் (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(modp, Class)) {
            எனில் (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<மாறிலி AstClass*, மாறிலி AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                மாறிலி auto hasEnclosingBackPointer = [](மாறிலி AstClass* covergroupp) {
                    திரும்பு covergroupp->exists([](மாறிலி AstVar* மாறிலி varp) {
                        மாறிலி AstClassRefDType* மாறிலி refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        திரும்பு refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* மாறிலி memberClassp, AstVar* மாறிலி varp) {
                        மாறிலி AstClassRefDType* மாறிலி refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        எனில் (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                மாறிலி string className = EmitCUtil::prefixNameProtect(classp);
                எனில் (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } இல்லையெனில் {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    எனில் (classp->hasRandVarsUpdate()) {
                        மாறிலி string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* மாறிலி updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    க்கு (மாறிலி EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    வெறுமை emitEnums(மாறிலி Ast_முனை_Module* modp) {
        தருக்க first = உண்மை;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            மாறிலி AstTypedef* மாறிலி tdefp = VN_CAST(nodep, Typedef);
            எனில் (!tdefp) தொடர்;
            எனில் (!tdefp->attrPublic()) தொடர்;
            மாறிலி AstEnumDType* மாறிலி edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            எனில் (!edtypep) தொடர்;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            எனில் (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } இல்லையெனில் {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                க்கு (மாறிலி AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(itemp->valuep(), Const)) {
                        எனில் (constp->இலக்கம்().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            தொடர்;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->இலக்கம்().emitC());
                    எனில் (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    வெறுமை emitStructDecl(மாறிலி Ast_முனை_Module* modp, Ast_முனை_UOr_Struct_DType* sdtypep,
                        std::set<Ast_முனை_UOr_Struct_DType*>& emitted) {
        எனில் (emitted.மொத்தம்(sdtypep) > 0) திரும்பு;
        emitted.செருகு(sdtypep);
        க்கு (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_முனை_UOr_Struct_DType* மாறிலி subp = itemp->getChildStructp();
            எனில் (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                எனில் (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        எனில் (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } இல்லையெனில் {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    எண்ணிக்கை வகுப்பு AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    முழு get_முனை_Attribute(மாறிலி AstMemberDType* itemp, AttributeType type) {
        மாறிலி தருக்க isArrayType = itemp->dtypep()->isNonPackedArray();
        தேர்வு (type) {
        நிலை AttributeType::WIDTH: {
            எனில் (isArrayType) {
                // For arrays, get innermost element width
                மாறிலி Ast_முனை_DType* dtype = itemp->dtypep();
                போது (dtype->subDTypep()) dtype = dtype->subDTypep();
                திரும்பு dtype->width();
            }
            திரும்பு itemp->width();
        }
        நிலை AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            திரும்பு isArrayType ? itemp->dtypep()->dimensions(உண்மை).second : 0;
        }
        இயல்பு: {
            திரும்பு 0;
        }
        }
    }
    template <AttributeType T>
    வெறுமை emitMemberVector(மாறிலி Ast_முனை_UOr_Struct_DType* sdtypep) {
        puts("return {");
        தருக்க needComma = பொய்;
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            எனில் (!itemp->isConstrainedRand()) தொடர்;
            // Comma handling: add before element except first
            எனில் (needComma) puts(",\n");
            putns(itemp, std::to_string(get_முனை_Attribute(itemp, T)));
            needComma = உண்மை;
        }
        puts("};\n}\n");
    }
    வெறுமை emitUnpackedUOrSBody(Ast_முனை_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), பொய், பொய்));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        எனில் (sdtypep->isConstrainedRand()) {
            தருக்க needComma = பொய்;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                எனில் (!itemp->isConstrainedRand()) தொடர்;
                எனில் (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = உண்மை;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = பொய்;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                எனில் (!itemp->isConstrainedRand()) தொடர்;
                எனில் (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", பொய், பொய்));
                needComma = உண்மை;
            }
            puts(">{};\n}\n");

            needComma = பொய்;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                எனில் (!itemp->isConstrainedRand()) தொடர்;
                எனில் (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = உண்மை;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            எனில் (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    வெறுமை emitVlAssign(மாறிலி Ast_முனை_DType* மாறிலி lhstype, மாறிலி Ast_முனை_DType* rhstype,
                      மாறிலி std::string& off, மாறிலி std::string& lhsdata,
                      மாறிலி std::string& rhsdata, தருக்க getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        எனில் (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } இல்லையெனில் {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        எனில் (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    வெறுமை emitPackedMember(மாறிலி Ast_முனை_DType* parentDtypep, மாறிலி Ast_முனை_DType* dtypep,
                          மாறிலி std::string& fieldname, மாறிலி std::string& offset, தருக்க getfunc,
                          மாறிலி std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        எனில் (மாறிலி auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            மாறிலி std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            மாறிலி std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            மாறிலி std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } இல்லையெனில் எனில் (VN_IS(dtypep, முனை_UOr_Struct_DType)) {
            மாறிலி std::string tmp = m_names.get("__Vtmp");
            எனில் (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } இல்லையெனில் {  // Emit `set` func
                மாறிலி std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } இல்லையெனில் {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            எனில் (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } இல்லையெனில் {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    வெறுமை emitPackedUOrSBody(Ast_முனை_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        மாறிலி AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        க்கு (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            எனில் (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        க்கு (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), பொய், பொய், உண்மை));
            puts(";\n");
        }

        மாறிலி std::string retArgName = m_names.get("__v");
        மாறிலி std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        எனில் (VN_IS(sdtypep, StructDType)) {
            க்கு (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/உண்மை, retArgName);
            }
        } இல்லையெனில் {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/உண்மை, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        எனில் (VN_IS(sdtypep, StructDType)) {
            க்கு (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/பொய், retArgName);
            }
        } இல்லையெனில் {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/பொய், retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    வெறுமை emitStructs(மாறிலி Ast_முனை_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_முனை_UOr_Struct_DType*> emitted;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            மாறிலி AstTypedef* மாறிலி tdefp = VN_CAST(nodep, Typedef);
            எனில் (!tdefp) தொடர்;
            Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), முனை_UOr_Struct_DType);
            எனில் (!sdtypep) தொடர்;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            மாறிலி தருக்க declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            எனில் (declaredInOtherModule) தொடர்;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    வெறுமை emitFuncDecls(மாறிலி Ast_முனை_Module* modp, தருக்க inClassBody) {
        std::vector<மாறிலி AstCFunc*> funcsp;

        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc)) {
                எனில் (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    தொடர்;
                எனில் (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    தொடர்;
                எனில் (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    தொடர்;
                எனில் (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    தொடர்;
                funcsp.தள்ளு_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](மாறிலி Ast_முனை* ap, மாறிலி Ast_முனை* bp) {
            திரும்பு ap->name() < bp->name();
        });

        க்கு (மாறிலி AstCFunc* மாறிலி funcp : funcsp) {
            எனில் (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    வெறுமை emitAll(மாறிலி Ast_முனை_Module* modp) {
        // Include files required by this AstNodeModule
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(modp, Class)) {
            க்கு (மாறிலி AstClassExtends* extp = classp->extendsp(); extp;
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
        எனில் (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(modp, Class)) {
            puts(" : ");
            எனில் (classp->extendsp()) {
                தருக்க needComma = பொய்;
                க்கு (மாறிலி AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    எனில் (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = உண்மை;
                }
            } இல்லையெனில் {
                puts("public virtual VlClass");
            }
        } இல்லையெனில் {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(பொய்);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ உண்மை);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ பொய்);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(மாறிலி Ast_முனை_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::உச்சி_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_உச்சி_Include();
        puts("#include \"verilated.h\"\n");
        எனில் (modp->is_உச்சி() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        எனில் (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        எனில் (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        எனில் (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        எனில் (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        எனில் (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        எனில் (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        எனில் (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.செருகு(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        எனில் (மாறிலி AstClassPackage* மாறிலி packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        க்கு (மாறிலி string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        எனில் (மாறிலி AstClassPackage* மாறிலி packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = இயல்பு;

பொது:
    நிலையான வெறுமை முதன்மை(மாறிலி Ast_முனை_Module* modp) { மாறிலி EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

வெறுமை V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    க்கு (மாறிலி Ast_முனை* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        எனில் (VN_IS(nodep, Class)) தொடர்;  // Declared with the ClassPackage
        மாறிலி Ast_முனை_Module* மாறிலி modp = VN_AS(nodep, முனை_Module);
        எனில் (modp->isConstPool()) தொடர்;  // Emitted by V3EmitCConstPool
        EmitCHeader::முதன்மை(modp);
    }
}
