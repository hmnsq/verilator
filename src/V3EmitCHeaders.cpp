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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCBase.h"
#شامل "V3File.h"
#شامل "V3MemberMap.h"
#شامل "V3UniqueNames.h"

#شامل <algorithm>
#شامل <cstdint>
#شامل <set>
#شامل <string>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

رده EmitCHeader final : عمومی EmitCBaseVisitorConst {
    V3UniqueNames m_names;
    VMemberMap m_memberMap;
    // METHODS

    رده Cover_شمار_Visitor final : عمومی VNVisitorConst {
        صحیح m_bins = 0;  // Running total of coverage bins counted so far

        تهی visit(Ast_گره_Cover_Decl* nodep) override {
            // Each module class owns the counters for declarations it emits;
            // duplicate no-inline instances need separate arrays for
            // forcePerInstance reporting.
            اگر (!nodep->dataDeclNullp()) m_bins += nodep->size();
        }
        تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    عمومی:
        explicit Cover_شمار_Visitor(ثابت Ast_گره_Module* modp) {
            iterateConst(const_cast<Ast_گره_Module*>(modp));
        }
        صحیح bins() ثابت { بازگشت m_bins; }
    };

    تهی decorateFirst(منطقی& first, ثابت string& str) {
        اگر (first) {
            putsDecoration(nullptr, str);
            first = نادرست;
        }
    }
    تهی emitCellDecls(ثابت Ast_گره_Module* modp) {
        منطقی first = درست;
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                اگر (cellp->modp()->isConstPool()) ادامه;  // Special emit rules
                decorateFirst(first, "// CELLS\n");
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* "
                                 + cellp->nameProtect() + ";\n");
            }
        }
    }
    تهی emitDesignVarDecls(ثابت Ast_گره_Module* modp) {
        // V3VariableOrder puts all variables at the front of the list: top level ports, then
        // trivially constructible variables, then the not trivially constructible ones, then
        // those that are not model state
        ثابت AstVar* portp = nullptr;
        ثابت AstVar* trivp = nullptr;
        ثابت AstVar* nontp = nullptr;
        size_t nPort = 0;
        size_t nTriv = 0;
        size_t nNont = 0;
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (!varp) بشکن;
            اگر (!varp->isModelState()) {
                // Cache line alignment is only effective on emitted fields
                UASSERT_OBJ(!varp->mtask_Cache_سطر_Align(), varp, "Aligned non-field");
            } وگرنه اگر (varp->isPrimaryIO()) {
                اگر (!portp) portp = varp;
                ++nPort;
            } وگرنه اگر (varp->isTriviallyConstructible()) {
                اگر (!trivp) trivp = varp;
                ++nTriv;
            } وگرنه {
                اگر (!nontp) nontp = varp;
                ++nNont;
            }
        }

        ثابت auto emitAndStep = [&](ثابت AstVar*& varpr) {
            emitVarDecl(varpr);
            varpr = VN_CAST(varpr->nextp(), Var);
        };

        // Emit primary ports
        اگر (nPort) {
            putsDecoration(nullptr, "\n// TOP LEVEL PORTS\n");
            // In anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls
            puts("union {\nstruct {\n");
            برای (size_t i = 0; i < nPort; ++i) emitAndStep(portp);
            puts("} __VprimaryPorts;\n};\n");
        }

        // Emit trivially constructible variables
        اگر (nTriv) {
            putsDecoration(nullptr, "\n// TRIVIALLY CONSTRUCTIBLE STATE\n");
            اگر (v3Global.opt.compLimitMembers() == 0) {
                // Anonymous structs disabled, so just emit them directly
                برای (size_t i = 0; i < nTriv; ++i) emitAndStep(trivp);
            } وگرنه {
                ثابت size_t lim = v3Global.opt.compLimitMembers();
                size_t anonL3s = 1;
                size_t anonL2s = 1;
                size_t anonL1s = 1;
                اگر (nTriv > (lim * lim * lim)) {
                    anonL3s = (nTriv + (lim * lim * lim) - 1) / (lim * lim * lim);
                    anonL2s = lim;
                    anonL1s = lim;
                } وگرنه اگر (nTriv > (lim * lim)) {
                    anonL2s = (nTriv + (lim * lim) - 1) / (lim * lim);
                    anonL1s = lim;
                } وگرنه اگر (nTriv > lim) {
                    anonL1s = (nTriv + lim - 1) / lim;
                }
                اگر (anonL1s != 1) {
                    putsDecoration(
                        nullptr,
                        "// Anonymous structures to workaround compiler member-count bugs\n");
                }
                size_t nEmitted = 0;
                برای (size_t l3 = 0; l3 < anonL3s && nEmitted < nTriv; ++l3) {
                    اگر (anonL3s != 1) puts("struct {\n");
                    برای (size_t l2 = 0; l2 < anonL2s && nEmitted < nTriv; ++l2) {
                        اگر (anonL2s != 1) puts("struct {\n");
                        برای (size_t l1 = 0; l1 < anonL1s && nEmitted < nTriv; ++l1) {
                            اگر (anonL1s != 1) puts("struct {\n");
                            برای (size_t l0 = 0; l0 < lim && nEmitted < nTriv; ++l0, ++nEmitted) {
                                emitAndStep(trivp);
                            }
                            اگر (anonL1s != 1) puts("};\n");
                        }
                        اگر (anonL2s != 1) puts("};\n");
                    }
                    اگر (anonL3s != 1) puts("};\n");
                }
                UASSERT_OBJ(nEmitted == nTriv, modp, "Not all variables emitted");
            }
        }

        // Emit not trivially constructible variables
        اگر (nNont) {
            putsDecoration(nullptr, "\n// NOT TRIVIALLY CONSTRUCTIBLE STATE\n");
            // Classes use real constructors
            اگر (VN_IS(modp, Class)) {
                برای (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
                بازگشت;
            }
            // Output under an anonymous union to prevent the C++ compiler from generating
            // constructor/destructor calls for them. They are managed explicitly in ctor/dtor.
            // While 'union { singleVar; }' for each variable would be simpler, it blows up
            // clang's compile time when using pre-compiled headers, joy.
            puts("union {\nstruct {\n");
            برای (size_t i = 0; i < nNont; ++i) emitAndStep(nontp);
            puts("} __Vnontrivs;\n};\n");
        }
    }
    تهی emitInternalVarDecls(ثابت Ast_گره_Module* modp) {
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            اگر (classp->needRNG()) {
                putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
                puts("VlRNGReseeds __Vm_rng;\n");
            }
        } وگرنه {  // not class
            putsDecoration(nullptr, "\n// INTERNAL VARIABLES\n");
            puts(EmitCUtil::symClassName() + "* vlSymsp;\n");
            puts("const char* vlNamep;\n");
            // Allocate object-local coverage counters only on modules that
            // contain emitted coverage declarations.
            ثابت صحیح coverBins = Cover_شمار_Visitor{modp}.bins();
            اگر (coverBins) {
                puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
                puts(" __Vcoverage[");
                puts(std::to_string(coverBins));
                puts("]{};\n");
            }
        }
    }
    تهی emitParamDecls(ثابت Ast_گره_Module* modp) {
        منطقی first = درست;
        برای (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isParam()) {
                    decorateFirst(first, "\n// PARAMETERS\n");
                    putns(varp, "static const ");
                    puts(varp->dtypep()->cType(varp->nameProtect(), نادرست, نادرست));
                    puts(";\n");
                }
            }
        }
    }
    تهی emitCtorDtorDecls(ثابت Ast_گره_Module* modp) {
        // Classes use CFuncs with isConstructor/isDestructor
        اگر (VN_IS(modp, Class)) بازگشت;

        // Proper modules use loose split ctor/dtor and are not copyable/movable
        ثابت std::string name = EmitCUtil::prefixNameProtect(modp);
        putsDecoration(nullptr, "\n// CONSTRUCTORS\n");
        putns(modp, name + "(" + EmitCUtil::symClassName() + "* symsp, const char* namep);\n");
        putns(modp, "~" + name + "();\n");
        putns(modp, "VL_UNCOPYABLE(" + name + ");\n");
        putns(modp, "VL_UNMOVABLE(" + name + ");\n");
    }
    تهی emitInternalMethodDecls(ثابت Ast_گره_Module* modp) {
        منطقی first = درست;
        ثابت string section = "\n// INTERNAL METHODS\n";

        اگر (!VN_IS(modp, Class)) {
            decorateFirst(first, section);
            اگر (v3Global.opt.coverage()) {
                puts("void " + protect("__Vconfigure") + "(bool first);\n");
            }
        } وگرنه {
            decorateFirst(first, section);
            ثابت std::string name = V3OutFormatter::quoteNameControls(
                VIdProtect::protect_واژگان_If(modp->prettyName(), v3Global.opt.protectIds()));
            // "override", but don't want clang to start checking them:
            puts("virtual const char* typeName() const { return \""s + name + "\"; }\n");
        }

        اگر (v3Global.opt.coverage() && !VN_IS(modp, Class)) {
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

        اگر (v3Global.opt.coverageToggle() && !VN_IS(modp, Class)) {
            decorateFirst(first, section);
            puts("void __vlCoverToggleInsert(int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp);\n");
        }

        اگر (v3Global.opt.savable()) {
            decorateFirst(first, section);
            puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
            puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
        }

        // Polymorphic clone for concrete (non-abstract, non-interface) classes
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            اگر (!classp->isInterfaceClass() && !classp->isVirtual()) {
                decorateFirst(first, section);
                using EmbeddedCovergroupVar = std::pair<ثابت AstClass*, ثابت AstVar*>;
                std::vector<EmbeddedCovergroupVar> embeddedCovergroupVars;
                ثابت auto hasEnclosingBackPointer = [](ثابت AstClass* covergroupp) {
                    بازگشت covergroupp->exists([](ثابت AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        بازگشت refp && refp->rawPointer();
                    });
                };
                const_cast<AstClass*>(classp)->foreachMember(
                    [&](AstClass* ثابت memberClassp, AstVar* ثابت varp) {
                        ثابت AstClassRefDType* ثابت refp
                            = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                        اگر (refp && refp->classp()->isCovergroup()
                            && hasEnclosingBackPointer(refp->classp())) {
                            embeddedCovergroupVars.emplace_back(memberClassp, varp);
                        }
                    });
                ثابت string className = EmitCUtil::prefixNameProtect(classp);
                اگر (embeddedCovergroupVars.empty() && !classp->hasRandVarsUpdate()) {
                    putns(classp,
                          "VlClass* clone() const { return new " + className + "(*this); }\n");
                } وگرنه {
                    putns(classp, "VlClass* clone() const { " + className + "* const clonep = new "
                                      + className + "(*this); ");
                    اگر (classp->hasRandVarsUpdate()) {
                        ثابت string updateName = "__VnoInFunc___VupdateRandVars";
                        AstCFunc* ثابت updatep
                            = VN_AS(m_memberMap.findMember(classp, updateName), CFunc);
                        UASSERT_OBJ(updatep, classp, "Missing updateRandVars method");
                        puts("clonep->" + updatep->nameProtect() + "();\n");
                    }
                    برای (ثابت EmbeddedCovergroupVar& item : embeddedCovergroupVars) {
                        puts("clonep->" + EmitCUtil::prefixNameProtect(item.first)
                             + "::" + item.second->nameProtect() + " = VlNull{}; ");
                    }
                    puts("return clonep; }\n");
                }
            }
        }
    }
    تهی emitEnums(ثابت Ast_گره_Module* modp) {
        منطقی first = درست;
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            اگر (!tdefp) ادامه;
            اگر (!tdefp->attrPublic()) ادامه;
            ثابت AstEnumDType* ثابت edtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), EnumDType);
            اگر (!edtypep) ادامه;
            decorateFirst(first, "\n// ENUMS (that were declared public)\n");
            اگر (edtypep->width() > 64) {
                putsDecoration(tdefp,
                               "// enum " + tdefp->nameProtect() + " ignored: Too wide for C++\n");
            } وگرنه {
                putns(tdefp, "enum " + tdefp->name() + " {\n");
                برای (ثابت AstEnumItem* itemp = edtypep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), EnumItem)) {
                    اگر (ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const)) {
                        اگر (constp->رقم().isFourState()) {
                            putns(itemp, "// " + itemp->nameProtect() + " is four-state\n");
                            ادامه;
                        }
                    }
                    putns(itemp, itemp->nameProtect());
                    puts(" = ");
                    putns(itemp, VN_AS(itemp->valuep(), Const)->رقم().emitC());
                    اگر (VN_IS(itemp->nextp(), EnumItem)) puts(",");
                    puts("\n");
                }
                puts("};\n");
            }
        }
    }
    تهی emitStructDecl(ثابت Ast_گره_Module* modp, Ast_گره_UOr_Struct_DType* sdtypep,
                        std::set<Ast_گره_UOr_Struct_DType*>& emitted) {
        اگر (emitted.شمار(sdtypep) > 0) بازگشت;
        emitted.درج(sdtypep);
        برای (AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_UOr_Struct_DType* ثابت subp = itemp->getChildStructp();
            اگر (subp && (!subp->packed() || sdtypep->packed())) {
                // Recurse if it belongs to the current module
                اگر (subp->classOrPackagep() == modp) {
                    emitStructDecl(modp, subp, emitted);
                    puts("\n");
                }
            }
        }
        اگر (sdtypep->packed()) {
            emitPackedUOrSBody(sdtypep);
        } وگرنه {
            emitUnpackedUOrSBody(sdtypep);
        }
    }
    شمارش رده AttributeType : uint8_t { WIDTH, DIMENSION };
    // Get member attribute based on type
    صحیح get_گره_Attribute(ثابت AstMemberDType* itemp, AttributeType type) {
        ثابت منطقی isArrayType = itemp->dtypep()->isNonPackedArray();
        انتخاب (type) {
        حالت AttributeType::WIDTH: {
            اگر (isArrayType) {
                // For arrays, get innermost element width
                ثابت Ast_گره_DType* dtype = itemp->dtypep();
                وقتی (dtype->subDTypep()) dtype = dtype->subDTypep();
                بازگشت dtype->width();
            }
            بازگشت itemp->width();
        }
        حالت AttributeType::DIMENSION: {
            // Return array dimension or 0 for non-arrays
            بازگشت isArrayType ? itemp->dtypep()->dimensions(درست).second : 0;
        }
        پیشفرض: {
            بازگشت 0;
        }
        }
    }
    template <AttributeType T>
    تهی emitMemberVector(ثابت Ast_گره_UOr_Struct_DType* sdtypep) {
        puts("return {");
        منطقی needComma = نادرست;
        برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (!itemp->isConstrainedRand()) ادامه;
            // Comma handling: add before element except first
            اگر (needComma) puts(",\n");
            putns(itemp, std::to_string(get_گره_Attribute(itemp, T)));
            needComma = درست;
        }
        puts("};\n}\n");
    }
    تهی emitUnpackedUOrSBody(Ast_گره_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");
        برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), نادرست, نادرست));
            puts(";\n");
        }

        // Three helper functions for struct constrained randomization:
        // - memberNames: Get member names
        // - getMembers: Access member references
        // - memberIndices: Retrieve member indices
        // - memberWidth: Retrieve member width
        // - memberDimension: Retrieve member dimension
        اگر (sdtypep->isConstrainedRand()) {
            منطقی needComma = نادرست;
            putns(sdtypep, "\nstd::vector<std::string> memberNames(void) const {\n");
            puts("return {");
            برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) ادامه;
                اگر (needComma) puts(",\n");
                putns(itemp, "\"" + itemp->shortName() + "\"");
                needComma = درست;
            }
            puts("};\n}\n");

            putns(sdtypep, "\nstd::vector<int> memberWidth(void) const {\n");
            emitMemberVector<AttributeType::WIDTH>(sdtypep);

            putns(sdtypep, "\nstd::vector<int> memberDimension(void) const {\n");
            emitMemberVector<AttributeType::DIMENSION>(sdtypep);

            needComma = نادرست;
            putns(sdtypep, "\nauto memberIndices(void) const {\n");
            puts("return std::index_sequence_for<");
            برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) ادامه;
                اگر (needComma) puts(",\n");
                putns(itemp, itemp->dtypep()->cType("", نادرست, نادرست));
                needComma = درست;
            }
            puts(">{};\n}\n");

            needComma = نادرست;
            putns(sdtypep, "\ntemplate <typename T>");
            putns(sdtypep, "\nauto getMembers(T& obj) {\n");
            puts("return std::tie(");
            برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                اگر (!itemp->isConstrainedRand()) ادامه;
                اگر (needComma) puts(",\n");
                putns(itemp, "obj." + itemp->nameProtect());
                needComma = درست;
            }
            puts(");\n}\n");
        }
        putns(sdtypep, "\nbool operator==(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return ");
        برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (itemp != sdtypep->membersp()) puts("\n    && ");
            putns(itemp, itemp->nameProtect() + " == " + "rhs." + itemp->nameProtect());
        }
        puts(";\n");
        puts("}\n");
        putns(sdtypep, "bool operator!=(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        puts("return !(*this == rhs);\n}\n");
        putns(sdtypep, "\nbool operator<(const " + EmitCUtil::prefixNameProtect(sdtypep)
                           + "& rhs) const {\n");
        برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
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
    تهی emitVlAssign(ثابت Ast_گره_DType* ثابت lhstype, ثابت Ast_گره_DType* rhstype,
                      ثابت std::string& off, ثابت std::string& lhsdata,
                      ثابت std::string& rhsdata, منطقی getfunc) {
        puts(getfunc ? "VL_ASSIGNSEL_" : "VL_SELASSIGN_");
        puts(lhstype->charIQWN());
        puts(rhstype->charIQWN());
        puts("(" + std::to_string(lhstype->width()) + ", ");  // LHS width
        اگر (getfunc) {
            puts(std::to_string(rhstype->width()) + ", ");  // Number of copy bits
            puts(off + ", ");  // LHS offset
        } وگرنه {
            // Number of copy bits. Use widthTototalBytes to
            // make VL_SELASSIGN_XX clear upper unused bits for us.
            // puts(std::to_string(lhstype->width()) + ", ");
            puts(std::to_string(lhstype->widthTotalBytes() * 8) + ", ");
        }
        puts(lhsdata + ", ");  // LHS data
        puts(rhsdata);  // RHS data
        اگر (!getfunc) {
            puts(", " + off);  // RHS offset
        }
        puts(");\n");
    }

    // `retOrArg` should be prefixed by `&` or suffixed by `.data()` depending on its type
    تهی emitPackedMember(ثابت Ast_گره_DType* parentDtypep, ثابت Ast_گره_DType* dtypep,
                          ثابت std::string& fieldname, ثابت std::string& offset, منطقی getfunc,
                          ثابت std::string& retOrArg) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت auto* adtypep = VN_CAST(dtypep, PackArrayDType)) {
            ثابت std::string index = m_names.get("__Vi");
            puts("for (int " + index + " = 0; " + index + " < "
                 + std::to_string(adtypep->elementsConst()) + "; ++" + index + ") {\n");

            ثابت std::string offsetInLoop
                = offset + " + " + index + " * " + std::to_string(adtypep->subDTypep()->width());
            ثابت std::string newName = fieldname + "[" + index + "]";
            emitPackedMember(parentDtypep, adtypep->subDTypep(), newName, offsetInLoop, getfunc,
                             retOrArg);
            puts("}\n");
        } وگرنه اگر (VN_IS(dtypep, گره_UOr_Struct_DType)) {
            ثابت std::string tmp = m_names.get("__Vtmp");
            اگر (getfunc) {  // Emit `get` func;
                // auto __tmp = field.get();
                puts("auto " + tmp + " = " + fieldname + ".get();\n");
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, tmp, getfunc);
            } وگرنه {  // Emit `set` func
                ثابت std::string tmptype = AstCDType::typeToHold(dtypep->width());
                // type tmp;
                puts(tmptype + " " + tmp + ";\n");
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, tmp, retOrArg, getfunc);
                // field.set(__tmp);
                puts(fieldname + ".set(" + tmp + ");\n");
            }
        } وگرنه {
            UASSERT_OBJ(VN_IS(dtypep, EnumDType) || VN_IS(dtypep, BasicDType), dtypep,
                        "Unsupported type in packed struct or union");
            اگر (getfunc) {  // Emit `get` func;
                // VL_ASSIGNSEL_XX(rbits, obits, lsb, lhsdata, rhsdata);
                emitVlAssign(parentDtypep, dtypep, offset, retOrArg, fieldname, getfunc);
            } وگرنه {  // Emit `set` func
                // VL_SELASSIGN_XX(rbits, obits, lhsdata, rhsdata, roffset);
                emitVlAssign(dtypep, parentDtypep, offset, fieldname, retOrArg, getfunc);
            }
        }
    }
    تهی emitPackedUOrSBody(Ast_گره_UOr_Struct_DType* sdtypep) {
        putns(sdtypep, sdtypep->verilogKwd());  // "struct"/"union"
        puts(" " + EmitCUtil::prefixNameProtect(sdtypep) + " {\n");

        AstMemberDType* itemp;
        AstMemberDType* lastItemp;
        ثابت AstMemberDType* witemp = nullptr;
        // LSB is first field in C, so loop backwards
        برای (lastItemp = sdtypep->membersp(); lastItemp && lastItemp->nextp();
             lastItemp = VN_AS(lastItemp->nextp(), MemberDType)) {
            اگر (lastItemp->width() == sdtypep->width()) witemp = lastItemp;
        }
        برای (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
            putns(itemp, itemp->dtypep()->cType(itemp->nameProtect(), نادرست, نادرست, درست));
            puts(";\n");
        }

        ثابت std::string retArgName = m_names.get("__v");
        ثابت std::string retArgType = AstCDType::typeToHold(sdtypep->width());

        // Emit `get` member function
        puts(retArgType + " get() const {\n");
        puts(retArgType + " " + retArgName + ";\n");
        اگر (VN_IS(sdtypep, StructDType)) {
            برای (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/درست, retArgName);
            }
        } وگرنه {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/درست, retArgName);
        }
        puts("return " + retArgName + ";\n");
        puts("}\n");

        // Emit `set` member function
        puts("void set(const " + retArgType + "& " + retArgName + ") {\n");
        اگر (VN_IS(sdtypep, StructDType)) {
            برای (itemp = lastItemp; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                emitPackedMember(sdtypep, itemp->dtypep(), itemp->nameProtect(),
                                 std::to_string(itemp->lsb()), /*getfunc=*/نادرست, retArgName);
            }
        } وگرنه {
            // We only need to fill the widest field of union
            emitPackedMember(sdtypep, witemp->dtypep(), witemp->nameProtect(),
                             std::to_string(witemp->lsb()), /*getfunc=*/نادرست, retArgName);
        }

        puts("}\n");

        puts("};\n");
        m_names.reset();
    }
    تهی emitStructs(ثابت Ast_گره_Module* modp) {
        // Track structs that've been emitted already
        std::set<Ast_گره_UOr_Struct_DType*> emitted;
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstTypedef* ثابت tdefp = VN_CAST(nodep, Typedef);
            اگر (!tdefp) ادامه;
            Ast_گره_UOr_Struct_DType* ثابت sdtypep
                = VN_CAST(tdefp->dtypep()->skipRefToEnump(), گره_UOr_Struct_DType);
            اگر (!sdtypep) ادامه;
            // V3Inline can copy one module into several parents. Only the module that
            // declares the struct emits it, so C++ sees only one definition.
            ثابت منطقی declaredInOtherModule = sdtypep->classOrPackagep() != modp;
            اگر (declaredInOtherModule) ادامه;
            emitStructDecl(modp, sdtypep, emitted);
        }
    }
    تهی emitFuncDecls(ثابت Ast_گره_Module* modp, منطقی inClassBody) {
        std::vector<ثابت AstCFunc*> funcsp;

        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) {
                اگر (funcp->dpiImportPrototype())  // Declared in __Dpi.h
                    ادامه;
                اگر (funcp->dpiExportDispatcher())  // Declared in __Dpi.h
                    ادامه;
                اگر (funcp->isMethod() != inClassBody)  // Only methods go inside class
                    ادامه;
                اگر (funcp->isMethod() && funcp->isLoose())  // Loose methods are declared lazily
                    ادامه;
                funcsp.افزودن_back(funcp);
            }
        }

        std::stable_sort(funcsp.begin(), funcsp.end(), [](ثابت Ast_گره* ap, ثابت Ast_گره* bp) {
            بازگشت ap->name() < bp->name();
        });

        برای (ثابت AstCFunc* ثابت funcp : funcsp) {
            اگر (inClassBody) ofp()->putsPrivate(funcp->declPrivate());
            emitCFuncDecl(funcp, modp);
        }
    }
    تهی emitAll(ثابت Ast_گره_Module* modp) {
        // Include files required by this AstNodeModule
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            برای (ثابت AstClassExtends* extp = classp->extendsp(); extp;
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
        اگر (!VN_IS(modp, Class)) puts("alignas(VL_CACHE_LINE_BYTES) ");
        puts(EmitCUtil::prefixNameProtect(modp));
        اگر (ثابت AstClass* ثابت classp = VN_CAST(modp, Class)) {
            puts(" : ");
            اگر (classp->extendsp()) {
                منطقی needComma = نادرست;
                برای (ثابت AstClassExtends* extp = classp->extendsp(); extp;
                     extp = VN_AS(extp->nextp(), ClassExtends)) {
                    اگر (needComma) puts(", ");
                    // Use virtual only for interfaces for class inheritance
                    // (extends)
                    puts(extp->classp()->useVirtualPublic() ? "virtual public " : "public ");
                    putns(extp, EmitCUtil::prefixNameProtect(extp->classp()));
                    needComma = درست;
                }
            } وگرنه {
                puts("public virtual VlClass");
            }
        } وگرنه {
            puts(" final");
        }
        puts(" {\n");
        ofp()->resetPrivate();
        ofp()->putsPrivate(نادرست);  // public:

        // Emit all class body contents
        emitCellDecls(modp);
        emitEnums(modp);
        emitDesignVarDecls(modp);
        emitInternalVarDecls(modp);
        emitParamDecls(modp);
        emitCtorDtorDecls(modp);
        emitInternalMethodDecls(modp);
        emitFuncDecls(modp, /* inClassBody: */ درست);

        // From `systemc_interface
        emitSystemCSection(modp, VSystemCSectionType::INT);

        // Close class body
        puts("};\n");
        // }}}

        // Emit out of class function declarations
        puts("\n");
        emitFuncDecls(modp, /* inClassBody: */ نادرست);
        emitSystemCSection(modp, VSystemCSectionType::HDR_POST);
    }

    // VISITORS
    تهی visit(Ast_گره* nodep) override { nodep->v3fatalSrc("Unused"); }  // LCOV_EXCL_LINE

    explicit EmitCHeader(ثابت Ast_گره_Module* modp) {
        UINFO(5, "  Emitting header for " << EmitCUtil::prefixNameProtect(modp));

        // Open output file
        openNewOutputHeaderFile(EmitCUtil::prefixNameProtect(modp), "Design internal header");
        puts("// See " + EmitCUtil::بالاترین_Class_Name() + ".h for the primary calling header\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_بالاترین_Include();
        puts("#include \"verilated.h\"\n");
        اگر (modp->is_بالاترین() && optSystemC()) puts("#include \"verilated_sc.h\"\n");
        اگر (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        اگر (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        اگر (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        اگر (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        اگر (v3Global.usesTiming()) puts("#include \"verilated_timing.h\"\n");
        اگر (v3Global.useRandomizeMethods()) puts("#include \"verilated_random.h\"\n");
        اگر (v3Global.usesForce()) puts("#include \"verilated_force.h\"\n");

        std::set<string> cuse_set;
        auto add_to_cuse_set = [&](string s) { cuse_set.درج(s); };

        forModCUse(modp, VUseType::INT_FWD_CLASS | VUseType::INT_INCLUDE, add_to_cuse_set);
        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            forModCUse(packagep->classp(), VUseType::INT_INCLUDE | VUseType::INT_FWD_CLASS,
                       add_to_cuse_set);
        }

        برای (ثابت string& s : cuse_set) puts(s);
        puts("\n");

        emitAll(modp);

        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            // Put the non-static class implementation in same h file for speed
            emitAll(packagep->classp());
        }

        ofp()->putsEndGuard();

        // Close output file
        closeOutputFile();
    }
    ~EmitCHeader() override = پیشفرض;

عمومی:
    ایستا تهی اصلی(ثابت Ast_گره_Module* modp) { ثابت EmitCHeader emitCHeader{modp}; }
};

//######################################################################
// EmitC class functions

تهی V3EmitC::emitcHeaders() {
    UINFO(2, __FUNCTION__ << ":");

    // Process each module in turn
    برای (ثابت Ast_گره* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        اگر (VN_IS(nodep, Class)) ادامه;  // Declared with the ClassPackage
        ثابت Ast_گره_Module* ثابت modp = VN_AS(nodep, گره_Module);
        اگر (modp->isConstPool()) ادامه;  // Emitted by V3EmitCConstPool
        EmitCHeader::اصلی(modp);
    }
}
