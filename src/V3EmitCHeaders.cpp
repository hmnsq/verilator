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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3File.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <algorithm>
#ضمن <cstdint>
#ضمن <set>
#ضمن <string>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

صنف EmitCHeader final : عام EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    صنف Cover_عدد_Visitor final : عام VNVisitorConst {
        صحيح m_bins = 0;  // Running total of coverage bins counted so far

        فراغ visit(Ast_عقدة_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            إذا (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        فراغ visit(Ast_عقدة* nodep) override { iterateChildrenConst(nodep); }

    عام:
        explicit Cover_عدد_Visitor(ثابت Ast_عقدة_Module* modp) {
            iterateConst(const_cast<Ast_عقدة_Module*>(modp));
        }
        صحيح bins() ثابت { أرجع m_bins; }
    };

    فراغ decorateFirst(منطقي& first, ثابت string& str) {
        إذا (first) {
            putsDecoration(nullptr, str);
            first = خطأ;
        }
    }
    فراغ emitCellDecls(ثابت Ast_عقدة_Module* modp) {
        منطقي first = true;
        لكل (ثابت Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                إذا (cellp->modp()->isConstPool()) استمر;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    فراغ emitDesignVarDecls(ثابت Ast_عقدة_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = nullptr;
        ثابت AstVar* trivp = nullptr;
        ثابت AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        لكل (ثابت Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) اكسر;
            إذا (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سطر_Align(), varp, "Aligned non-field");
            } وإلا إذا (varp->isPrimaryIO()) {
                إذا (!portp) portp = varp;
                ++nPort;
            } وإلا إذا (varp->isTriviallyConstructible()) {
                إذا (!trivp) trivp = varp;
                ++nTriv;
            } وإلا {
                إذا (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت auto emitAndStep = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        إذا (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            لكل (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        إذا (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            إذا (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                لكل (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } وإلا {
                ثابت size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                إذا (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } وإلا إذا (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } وإلا إذا (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                إذا (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                لكل (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    إذا (anonL3s != 1) puts("struct {\n");
                    لكل (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        إذا (anonL2s != 1) puts("struct {\n");
                        لكل (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            إذا (anonL1s != 1) puts("struct {\n");
                            لكل (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            إذا (anonL1s != 1) puts("};\n");
                        }
                        إذا (anonL2s != 1) puts("};\n");
                    }
                    إذا (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        إذا (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            إذا (VN_IS(modp, Class)) {
                لكل (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                أرجع;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            لكل (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    فراغ emitInternalVarDecls(ثابت Ast_عقدة_Module* modp) {
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } وإلا {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحيح coverBins = Cover_عدد_Visitor{modp}.bins();
            إذا (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    فراغ emitParamDecls(ثابت Ast_عقدة_Module* modp) {
        منطقي first = true;
        لكل (Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), خطأ, خطأ));
                    puts(";\n");
                }
            }
        }
    }
    فراغ emitCtorDtorDecls(ثابت Ast_عقدة_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        إذا (VN_IS(modp, Class)) أرجع;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    فراغ emitInternalMethodDecls(ثابت Ast_عقدة_Module* modp) {
        منطقي first = true;
        ثابت string section = "\n// INTERNAL METHODS\n";

        إذا (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            إذا (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } وإلا {
            decorateFirst(first, section);
            ثابت std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_كلمات_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        إذا (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        إذا (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        إذا (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            إذا (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت auto hasEnclosingBackPointer = [](ثابت AstClass* covergroupp) {
                    أرجع covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        أرجع refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        إذا (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::prefixNameProtect(classp);
                إذا (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } وإلا {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    إذا (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    لكل (ثابت EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    فراغ emitEnums(ثابت Ast_عقدة_Module* modp) {
        منطقي first = true;
        لكل (ثابت Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            إذا (!tdefp->attrPublic()) استمر;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            إذا (!edtypep) استمر;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            إذا (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } وإلا {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                لكل (ثابت AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    إذا (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        إذا (constp->رقم().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            استمر;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->رقم().emitC());
                    إذا (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    فراغ emitStructDecl(ثابت Ast_عقدة_Module* modp, Ast_عقدة_UOr_Struct_DType* sdtypep,
                        std::set<Ast_عقدة_UOr_Struct_DType*>& emitted) {
        إذا (emitted.عدد(sdtypep) > 0) أرجع;
        emitted.أدرج(sdtypep);
        لكل (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_عقدة_UOr_Struct_DType* ثابت subp = itemp->getChildStructp();
            إذا (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                إذا (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        إذا (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } وإلا {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    تعداد صنف AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    صحيح get_عقدة_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقي isArrayType = itemp->dtypep()->isNonPackedArray();
        بدل (type) {
        حالة AttributeType::WIDTH: {
            إذا (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_عقدة_DType* dtype = itemp->dtypep();
                طالما (dtype->subDTypep()) dtype = dtype->subDTypep();
                أرجع dtype->width();
            }
            أرجع itemp->width();
        }
        حالة AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            أرجع isArrayType ? itemp->dtypep()->dimensions(true).second : 0;
        }
        افتراضي: {
            أرجع 0;
        }
        }
    }
    template <AttributeType T>
    فراغ emitMemberVector(ثابت Ast_عقدة_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقي needComma = خطأ;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!itemp->isConstrainedRand()) استمر;
            // Comma handling: add before element except first
            إذا (needComma) puts(",\n");
            putns(itemp, std::to_string(get_عقدة_Attribute(itemp, T)));
            needComma = true;
        }
        puts("};\n}\n");
    }
    فراغ emitUnpackedUOrSBody(Ast_عقدة_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        إذا (sdtypep->isConstrainedRand()) {
            منطقي needComma = خطأ;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = true;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = خطأ;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", خطأ, خطأ));
                needComma = true;
            }
            puts(">{};\n}\n");

            needComma = خطأ;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                إذا (!itemp->isConstrainedRand()) استمر;
                إذا (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = true;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    فراغ emitVlAssign(ثابت Ast_عقدة_DType* ثابت lhstype, ثابت Ast_عقدة_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقي getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        إذا (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } وإلا {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        إذا (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    فراغ emitPackedMember(ثابت Ast_عقدة_DType* parentDtypep, ثابت Ast_عقدة_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& offset, منطقي getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ثابت std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ثابت std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } وإلا إذا (VN_IS(dtypep, عقدة_UOr_Struct_DType)) {
            ثابت std::string tmp = m_names.get("__Vtmp");
            إذا (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } وإلا {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } وإلا {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            إذا (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } وإلا {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    فراغ emitPackedUOrSBody(Ast_عقدة_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ثابت AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        لكل (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            إذا (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        لكل (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), خطأ, خطأ, true));
            puts(";\n");
        }

        ثابت std::string retArgName = m_names.get("__v");
        ثابت std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/true, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/true, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        إذا (VN_IS(sdtypep, StructDType)) {
            لكل (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/خطأ, retArgName);
            }
        } وإلا {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/خطأ, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    فراغ emitStructs(ثابت Ast_عقدة_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_عقدة_UOr_Struct_DType*> emitted;
        لكل (ثابت Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            إذا (!tdefp) استمر;
            Ast_عقدة_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), عقدة_UOr_Struct_DType);
            إذا (!sdtypep) استمر;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقي declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            إذا (declaredInOtherModule) استمر;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    فراغ emitFuncDecls(ثابت Ast_عقدة_Module* modp, منطقي inClassBody) {
        std::vector<ثابت AstCFunc*> funcsp;

        لكل (ثابت Ast_عقدة* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                إذا (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    استمر;
                إذا (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    استمر;
                إذا (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    استمر;
                funcsp.ادفع_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ثابت Ast_عقدة* ap, ثابت Ast_عقدة* bp) {
            أرجع ap->name() < bp->name();
        });

        لكل (ثابت AstCFunc* ثابت funcp : funcsp) {
            إذا (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    فراغ emitAll(ثابت Ast_عقدة_Module* modp) {
        // Include files required by this AstNodeModule
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
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
        إذا (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        إذا (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            إذا (classp->extendsp()) {
                منطقي needComma = خطأ;
                لكل (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    إذا (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = true;
                }
            } وإلا {
                puts("public virtual VlClass");
            }
        } وإلا {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(خطأ);  // public:

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
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ خطأ);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    فراغ visit(Ast_عقدة* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ثابت Ast_عقدة_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::قمة_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_قمة_Include();
        puts("#include \"verilated.h\"\n");
        إذا (modp->is_قمة() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        إذا (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        إذا (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        إذا (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        إذا (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        إذا (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        إذا (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        إذا (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.أدرج(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        لكل (ثابت string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        إذا (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = افتراضي;

عام:
    ساكن فراغ main(ثابت Ast_عقدة_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    لكل (ثابت Ast_عقدة* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Class)) استمر;  // Declared with the ClassPackage
        ثابت Ast_عقدة_Module* ثابت modp = VN_AS(nodep, عقدة_Module);
        إذا (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
        EmitCHeader::main(modp);
    }
}
