// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Replicate modules for parameterization
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
// PARAM TRANSFORMATIONS:
//   Top down traversal:
//      For each cell:
//          If parameterized,
//              Determine all parameter widths, constant values.
//              (Interfaces also matter, as if a module is parameterized
//              this effectively changes the width behavior of all that
//              reference the iface.)
//
//              Clone module cell calls, renaming with __{par1}_{par2}_...
//              Substitute constants for cell's module's parameters.
//              Relink pins and cell and ifacerefdtype to point to new module.
//
//              For interface Parent's we have the AstIfaceRefDType::cellp()
//              pointing to this module.  If that parent cell's interface
//              module gets parameterized, AstIfaceRefDType::cloneRelink
//              will update AstIfaceRefDType::cellp(), and V3LinkDot will
//              see the new interface.
//
//              However if a submodule's AstIfaceRefDType::ifacep() points
//              to the old (unparameterized) interface and needs correction.
//              To detect this we must walk all pins looking for interfaces
//              that the parent has changed and propagate down.
//
//          Then process all modules called by that cell.
//          (Cells never referenced after parameters expanded must be ignored.)
//
//      Arrayed instances of modules and interfaces are expanded into their
//      elements before the above, each element knowing its position in the
//      array (for V3Width to select its part of the port connections).
//      Interface array ports are split into a variable for each element.
//
//   After we complete parameters, the varp's will be wrong (point to old module)
//   and must be relinked.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Param.h"

#ضمن "V3Case.h"
#ضمن "V3Const.h"
#ضمن "V3EmitV.h"
#ضمن "V3HierBlock.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Os.h"
#ضمن "V3Parse.h"
#ضمن "V3Simulate.h"
#ضمن "V3Stats.h"
#ضمن "V3Unroll.h"
#ضمن "V3Width.h"

#ضمن <cctype>
#ضمن <cstdlib>
#ضمن <deque>
#ضمن <map>
#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ساكن AstClassRefDType* class_Ref_DType_Of_نود(Ast_نود* nodep) {
    طالما (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_نود_DType* ثابت dtp = VN_CAST(nodep, نود_DType);
    أرجع dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

صنف ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ثابت V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ثابت std::string, Ast_نود_Module*>;
    using ParamConstMap = std::map<ثابت std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ثابت std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ثابت منطقي m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<ثابت V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ثابت std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<ثابت AstVar*> m_untypedParams;

    // METHODS

عام:
    ParameterizedHierBlocks(ثابت V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        لكل (ثابت auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ثابت V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            لكل (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    جديد File_باريس{File_باريس::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ثابت منطقي inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.سيسيڤ({hierOpt.second.origName(), {}});
        }
        لكل (Ast_نود_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), نود_Module)) {
            إذا (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ثابت string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ثابت auto defParamIt = m_modParams.find(actualModName);
            إذا (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                        إذا (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ثابت AstBasicDType* ثابت dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        إذا (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    منطقي hierSubRun() ثابت { أرجع m_hierSubRun; }
    منطقي isHierBlock(ثابت string& origName) ثابت {
        أرجع m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_نود_Module* findByParams(ثابت string& origName, AstPin* firstPinp,
                                ثابت Ast_نود_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ثابت std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        ثابت auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ثابت AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        لكل (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            منطقي found = true;
            size_t paramIdx = 0;
            ثابت ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            لكل (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;
                إذا (ثابت AstVar* ثابت modvarp = pinp->modVarp()) {
                    AstConst* ثابت constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ثابت auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ثابت منطقي untyped = m_untypedParams.کيراءن(paramIt->second);
                    AstConst* ثابت defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    إذا (defValuep && areSame(constp, defValuep)) {
                        إذا (!untyped || constp->same_نيلاي_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            استمر;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ثابت auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    إذا (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = خطأ;
                        اكسر;
                    }
                    إذا (untyped && !constp->same_نيلاي_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = خطأ;
                        اكسر;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            إذا (found && paramIdx == hierIt->second->params().size()) اكسر;
        }
        إذا (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_نود::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            أرجع nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ثابت HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ثابت auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        إذا (it == m_hierBlockMod.end()) أرجع nullptr;
        أرجع it->second;
    }
    ساكن منطقي areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        إذا (pinValuep->isString()) {
            أرجع pinValuep->اڠک().toString() == hierOptParamp->اڠک().toString();
        }

        إذا (hierOptParamp->isDouble()) {
            مزدوج var;
            إذا (pinValuep->isDouble()) {
                var = pinValuep->اڠک().toDouble();
            } وإلا {  // Cast from integer to real
                V3_نومبور var_اڠک{pinValuep, V3_نومبور::Double{}, 0.0};
                var_اڠک.opIToRD(pinValuep->اڠک());
                var = var_اڠک.toDouble();
            }
            أرجع V3_نومبور::epsilonEqual(var, hierOptParamp->اڠک().toDouble());
        } وإلا {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_نومبور var_اڠک{pinValuep, hierOptParamp->اڠک().width()};
            إذا (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_اڠک.opRToIRoundS(pinValuep->اڠک());
            } وإلا إذا (pinValuep->isSigned()) {
                var_اڠک.opExtendS(pinValuep->اڠک(), pinValuep->اڠک().width());
            } وإلا {
                var_اڠک.opAssign(pinValuep->اڠک());
            }
            V3_نومبور isEq{pinValuep, 1};
            isEq.opEq(var_اڠک, hierOptParamp->اڠک());
            أرجع isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

صنف ParamProcessor final {
    // NODE STATE - Local
    //   AstVar::user3()        // int    Global parameter number (for naming new module)
    //                          //        (0=not processed, 1=iterated, but no number,
    //                          //         65+ parameter numbered)
    // NODE STATE - Shared with ParamVisitor
    //   AstNodeModule::user3p()  // AstNodeModule* Unaltered copy of the parameterized module.
    //                               The module/class node may be modified according to parameter
    //                               values and an unchanged copy is needed to instantiate
    //                               modules/classes with different parameters.
    //   AstNodeModule::user2() // bool   True if processed
    //   AstGenFor::user2()     // bool   True if processed
    //   AstVar::user2()        // bool   True if constant propagated
    //   AstIfaceRefDType::user2()      // bool   True if visited
    //   AstClassRefDType::user2()      // bool   True if visited
    //   AstNodeModule:user4p() // AstNodeModule* Parametrized copy with default parameters
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;
    ثابت VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<ثابت Ast_نود*, Ast_نود*>;
    بنية ModInfo final {
        Ast_نود_Module* ثابت m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_نود_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ثابت std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ثابت std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ثابت std::string, std::string> m_نيلاي_Names;  // Parameter value text to its name
    std::set<std::string> m_نيلاي_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ساكن constexpr std::string::size_type DIGEST_DIGITS = 8;

    ثابت Ast_نود_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_نيلاي_Map = std::map<std::string, Ast_نود*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_نود_Module*, Default_نيلاي_Map> m_default_Parameter_نيلاي_سموا;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ثابت Ast_نود_Module*, ثابت AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, صحيح>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, صحيح> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ثابت AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ساكن فراغ makeSmallNames(Ast_نود_Module* modp) {
        std::vector<صحيح> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam() || varp->isIfaceRef()) {
                    حرف ch = varp->name()[0];
                    ch = std::toupper(ch);
                    إذا (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<صحيح>(ch)] * 256 + ch);
                    usedLetter[static_cast<صحيح>(ch)]++;
                }
            } وإلا إذا (AstParamTypeDType* ثابت typep = VN_CAST(stmtp, ParamTypeDType)) {
                ثابت حرف ch = 'T';
                typep->user3(usedLetter[static_cast<صحيح>(ch)] * 256 + ch);
                usedLetter[static_cast<صحيح>(ch)]++;
            }
        }
    }
    ساكن string paramSmallName(Ast_نود_Module* modp, Ast_نود* varp) {
        إذا (varp->user3() <= 1) makeSmallNames(modp);
        صحيح index = varp->user3() / 256;
        ثابت حرف ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        طالما (index) {
            st += cvtToStr(static_cast<حرف>((index % 25) + 'A'));
            index /= 26;
        }
        أرجع st;
    }

    ساكن منطقي is_Aggregate_Param_نيلاي(ثابت Ast_نود* nodep) {
        أرجع VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ساكن string param_نيلاي_String(ثابت Ast_نود* nodep) {
        إذا (ثابت AstRefDType* ثابت refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string کونچي = nodep->name();
        إذا (ثابت AstIfaceRefDType* ثابت ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                کونچي = ifrtp->cellp()->modp()->name();
            } وإلا إذا (ifrtp->ifacep()) {
                کونچي = ifrtp->ifacep()->name();
            } وإلا {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } وإلا إذا (ثابت Ast_نود_UOr_Struct_DType* ثابت dtypep
                   = VN_CAST(nodep, نود_UOr_Struct_DType)) {
            کونچي += " ";
            کونچي += dtypep->verilogKwd();
            کونچي += " {";
            لكل (ثابت Ast_نود* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                کونچي += param_نيلاي_String(memberp);
                کونچي += ";";
            }
            کونچي += "}";
        } وإلا إذا (ثابت AstMemberDType* ثابت dtypep = VN_CAST(nodep, MemberDType)) {
            کونچي += " ";
            کونچي += param_نيلاي_String(dtypep->subDTypep());
        } وإلا إذا (ثابت AstBasicDType* ثابت dtypep = VN_CAST(nodep, BasicDType)) {
            إذا (dtypep->isSigned()) کونچي += " signed";
            إذا (dtypep->isRanged()) {
                کونچي += "[" + cvtToStr(dtypep->کيري()) + ":" + cvtToStr(dtypep->کانن()) + "]";
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت dtypep = VN_CAST(nodep, PackArrayDType)) {
            کونچي += "[";
            کونچي += cvtToStr(dtypep->کيري());
            کونچي += ":";
            کونچي += cvtToStr(dtypep->کانن());
            کونچي += "] ";
            کونچي += param_نيلاي_String(dtypep->subDTypep());
        } وإلا إذا (ثابت AstPattern* ثابت patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            کونچي += "'{";
            لكل (ثابت AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                إذا (memp->isDefault()) کونچي += "default";
                إذا (ثابت AstText* ثابت textp = VN_CAST(memp->keyp(), Text)) {
                    کونچي += textp->text();
                } وإلا إذا (memp->keyp()) {
                    کونچي += param_نيلاي_String(memp->keyp());
                }
                کونچي += ":";
                إذا (memp->repp()) کونچي += param_نيلاي_String(memp->repp()) + "x";
                لكل (ثابت Ast_نود_اوڠکاڤن* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), نود_اوڠکاڤن)) {
                    کونچي += param_نيلاي_String(valuep) + ";";
                }
                کونچي += ",";
            }
            کونچي += "}";
        } وإلا إذا (ثابت AstConsArray* ثابت consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            کونچي += "{";
            لكل (ثابت AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                کونچي += cvtToStr(itemp->index()) + ":" + param_نيلاي_String(itemp->valuep()) + ",";
            }
            إذا (consArrayp->defaultp()) {
                کونچي += "default:" + param_نيلاي_String(consArrayp->defaultp()) + ",";
            }
            کونچي += "}";
        } وإلا إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep, ConsStruct)) {
            کونچي += "{";
            لكل (ثابت AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                کونچي += param_نيلاي_String(itemp->valuep());
                کونچي += ",";
            }
            کونچي += "}";
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            إذا (classRefp->classp()) {
                ثابت string& className = classRefp->classp()->name();
                ثابت string& origName = classRefp->classp()->origName();
                ثابت منطقي isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                إذا (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } وإلا إذا (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    کونچي = className;
                } وإلا {
                    // Unspecialized class with no params - use origName
                    کونچي = origName;
                }
            } وإلا {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            کونچي = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } وإلا إذا (ثابت AstQueueDType* ثابت dtypep = VN_CAST(nodep, QueueDType)) {
            کونچي = paramElemString(dtypep->subDTypep()) + "$[$";
            إذا (dtypep->boundConst()) کونچي += ":" + cvtToStr(dtypep->boundConst());
            کونچي += "]";
        } وإلا إذا (ثابت AstDynArrayDType* ثابت dtypep = VN_CAST(nodep, DynArrayDType)) {
            کونچي = paramElemString(dtypep->subDTypep()) + "$[]";
        } وإلا إذا (ثابت AstAssocArrayDType* ثابت dtypep = VN_CAST(nodep, AssocArrayDType)) {
            کونچي = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->کونچي_DTypep()) + "]";
        } وإلا إذا (ثابت AstWildcardArrayDType* ثابت dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            کونچي = paramElemString(dtypep->subDTypep()) + "$[*]";
        } وإلا إذا (ثابت AstEnumDType* ثابت dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            کونچي += " enum {";
            لكل (ثابت AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const);
                کونچي += itemp->name() + "=" + (constp ? constp->اڠک().ascii() : "?") + ";";
            }
            کونچي += "}";
        } وإلا إذا (ثابت Ast_نود_DType* ثابت dtypep = VN_CAST(nodep, نود_DType)) {
            کونچي += dtypep->prettyDTypeName(true);
        }
        UASSERT_OBJ(!کونچي.empty(), nodep, "Parameter yielded no value string");
        أرجع کونچي;
    }
    // As paramValueString(), for the element or key type of a container
    ساكن string paramElemString(ثابت Ast_نود_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ثابت AstClassRefDType* ثابت classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        إذا (classRefp && classRefp->paramsp()) أرجع classRefp->prettyDTypeName(true);
        أرجع param_نيلاي_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ساكن string digestSuffix(ثابت string& text, std::set<string>& usedr) {
        ثابت string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        طالما (digits < hex.size() && !usedr.سيسيڤ(hex.substr(0, digits)).second) ++digits;
        أرجع hex.substr(0, digits);
    }
    string param_نيلاي_نومبور(Ast_نود* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        إذا (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ثابت string text = param_نيلاي_String(nodep);
        ثابت auto pair = m_نيلاي_Names.emplace(text, "");
        إذا (pair.second) pair.first->second = "z" + digestSuffix(text, m_نيلاي_Suffixes);
        أرجع pair.first->second;
    }
    string moduleCalcName(ثابت Ast_نود_Module* srcModp, ثابت string& longname) {
        string newname = longname;
        إذا (longname.length() > 30) {
            ثابت auto pair = m_longMap.emplace(longname, "");
            إذا (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        أرجع newname;
    }
    ساكن Ast_نود_DType* arraySubDTypep(Ast_نود_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        إذا (ثابت AstBracketArrayDType* ثابت adtypep = VN_CAST(nodep, BracketArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        أرجع nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ساكن Ast_نود_DType* arraySubDTypeDeepp(Ast_نود_DType* nodep) {
        طالما (Ast_نود_DType* ثابت subp = arraySubDTypep(nodep)) nodep = subp;
        أرجع nodep;
    }
    ساكن منطقي isString(Ast_نود_DType* nodep) {
        إذا (AstBasicDType* ثابت basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            أرجع basicp->isString();
        أرجع خطأ;
    }
    فراغ collectPins(CloneMap* clonemapp, Ast_نود_Module* modp, منطقي originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت Ast_نود* originalParamp = nullptr;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            إذا (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    فراغ relinkPins(ثابت CloneMap* clonemapp, AstPin* startpinp) {
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ثابت auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } وإلا إذا (pinp->modPTypep()) {
                ثابت auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } وإلا {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    فراغ relinkPinsByName(AstPin* startpinp, Ast_نود_Module* modp) {
        std::map<ثابت string, AstVar*> nameToPin;
        لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                ثابت auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_نود_Module* modp,
                                       std::set<ثابت Ast_نود_Module*>& visited) {
        إذا (!visited.سيسيڤ(modp).second) أرجع خطأ;
        لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت cellp = VN_CAST(stmtp, Cell);
            إذا (!cellp) استمر;
            لكل (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    أرجع true;
                }
            }
            إذا (Ast_نود_Module* ثابت childModp = cellp->modp()) {
                إذا (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    أرجع true;
                }
            }
        }
        visited.erase(modp);
        أرجع خطأ;
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_نود_Module* modp) {
        std::set<ثابت Ast_نود_Module*> visited;
        أرجع hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ثابت AstPin* nestedDefparamp(ثابت Ast_نود_Module* modp) {
        ثابت auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        إذا (!pair.second) أرجع pair.first->second;  // Searched, or being searched
        ثابت AstPin* foundp = nullptr;
        modp->exists([&](ثابت AstCell* cellp) {
            لكل (ثابت AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->paramPath().empty()) foundp = pinp;
            }
            إذا (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            أرجع foundp != nullptr;
        });
        pair.first->second = foundp;
        أرجع foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    فراغ checkSupportedParam(Ast_نود_Module* modp, AstPin* pinp) ثابت {
        // ConsArray is not supported because that can not be set via -G
        // option.
        إذا (pinp->modVarp()) {
            منطقي supported = خطأ;
            إذا (ثابت AstConst* ثابت constp = VN_CAST(pinp->exprp(), Const)) {
                supported = true;
                إذا (constp->اڠک().isString()
                    && !V3HierBlock::stringParamPassable(constp->اڠک().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            إذا (!supported) {
                pinp->v3error(
                    Ast_نود::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } وإلا إذا (ثابت AstParamTypeDType* ثابت typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_نود::prettyNameQ(modp->origName()));
        }
    }
    منطقي moduleExists(ثابت string& modName) ثابت {
        إذا (m_allModuleNames.find(modName) != m_allModuleNames.end()) أرجع true;
        إذا (m_modNameMap.find(modName) != m_modNameMap.end()) أرجع true;
        أرجع خطأ;
    }

    string parameterizedHierBlockName(Ast_نود_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_نود*> pins;

        AstPin* pinp = paramPinsp;
        طالما (pinp) {
            checkSupportedParam(modp, pinp);
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                إذا (!pinp->exprp()) استمر;
                إذا (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } وإلا إذا (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), نود_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ثابت auto pair = m_default_Parameter_نيلاي_سموا.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        إذا (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            إذا (ثابت AstPin* ثابت defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_نود::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_نيلاي_Map params;
            لكل (Ast_نود* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isGParam()) {
                        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } وإلا إذا (AstParamTypeDType* ثابت p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_نود* ثابت dtypep = static_cast<Ast_نود*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ثابت auto paramsIt = pair.first;
        إذا (paramsIt->second.empty()) أرجع modp->origName();  // modp has no parameter

        string longname = modp->origName();
        لكل (auto&& default_نيلاي : paramsIt->second) {
            ثابت auto pinIt = pins.find(default_نيلاي.first);
            // If the pin does not have a value assigned, use the default one.
            ثابت Ast_نود* ثابت nodep = pinIt == pins.end() ? default_نيلاي.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_نيلاي.first + "=";
            // constp can be nullptr

            إذا (ثابت AstConst* ثابت p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->اڠک().ascii(خطأ);
            } وإلا إذا (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ڤوکوق(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ثابت auto iter = m_longMap.find(longname);
        إذا (iter != m_longMap.end()) أرجع iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.سيسيڤ(longname);
        طالما (true) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ثابت string hashStr = hashStrGen.digestHex();
            لكل (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                إذا (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                إذا (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    أرجع newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.سيسيڤ(V3Os::trueRandom(64));
        }
    }
    فراغ replaceRefsRecurse(Ast_نود* ثابت nodep, ثابت AstClass* ثابت oldClassp,
                            AstClass* ثابت newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            إذا (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            إذا (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            إذا (typedefp->subDTypep()) {
                إذا (AstClassRefDType* ثابت classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    إذا (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } وإلا إذا (Ast_نود_FTask_Ref* ثابت ftaskRefp = VN_CAST(nodep, نود_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            إذا (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            إذا (Ast_نود_FTask* ثابت oldTaskp = ftaskRefp->taskp()) {
                إذا (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        إذا (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        إذا (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        إذا (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        إذا (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        إذا (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    صنف VarXRefRelinkVisitor final : عام VNVisitor {
        Ast_نود_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_نود_Module*> m_varModuleMap;  // Cache var->module lookups

    عام:
        explicit VarXRefRelinkVisitor(Ast_نود_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    خاص:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_نود_Module* findVarModule(AstVar* varp) {
            ثابت auto it = m_varModuleMap.find(varp);
            إذا (it != m_varModuleMap.end()) أرجع it->second;
            Ast_نود_Module* varModp = nullptr;
            لكل (Ast_نود* np = varp; np; np = np->backp()) {
                إذا (Ast_نود_Module* ثابت modp = VN_CAST(np, نود_Module)) {
                    varModp = modp;
                    اكسر;
                }
            }
            m_varModuleMap[varp] = varModp;
            أرجع varModp;
        }

        فراغ visit(AstVarXRef* nodep) override {
            AstVar* ثابت varp = nodep->varp();
            إذا (!varp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ثابت string& dotted = nodep->dotted();
            إذا (dotted.empty()) {
                iterateChildren(nodep);
                أرجع;
            }

            ثابت size_t dotPos = dotted.find('.');
            ثابت string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            إذا (portName.empty()) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            لكل (Ast_نود* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varChkp = VN_CAST(stmtp, Var)) {
                    إذا (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        اكسر;
                    }
                }
            }
            إذا (!portVarp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ثابت irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            إذا (!irefp) {
                iterateChildren(nodep);
                أرجع;
            }

            Ast_نود_Module* ثابت newIfacep = irefp->ifaceViaCellp();
            إذا (!newIfacep) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_نود_Module* ثابت varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            إذا (varModp && varModp != newIfacep) {
                لكل (Ast_نود* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت newVarp = VN_CAST(stmtp, Var)) {
                        إذا (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            اكسر;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    فراغ fixupCrossModuleRefDTypes(Ast_نود_Module* newModp) {
        إذا (!V3LinkDotIfaceCapture::enabled()) أرجع;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        صحيح fixed_کيراءن = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ثابت VIfaceCaptureTag* ثابت tagp = V3LinkDotIfaceCapture::captureTag(refp);
            إذا (!tagp) أرجع;

            Ast_نود_Module* ثابت correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            إذا (!correctModp || correctModp->dead()) أرجع;
            إذا (correctModp->parameterizedTemplate()) أرجع;

            منطقي fixed = خطأ;
            إذا (refp->typedefp()) {
                إذا (AstTypedef* ثابت newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    إذا (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = true;
                }
            }
            إذا (!fixed && refp->refDTypep()) {
                إذا (Ast_نود_DType* ثابت newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = true;
                }
            }
            إذا (fixed) ++fixed_کيراءن;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_کيراءن);
    }

    // Return true on success, false on error
    منطقي deepCloneModule(Ast_نود_Module* srcModp, Ast_نود* ifErrorp, AstPin* paramsp,
                         ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_نود_Module* newModp;
        إذا (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ڤوکوق(خطأ), نود_Module);
        } وإلا {
            newModp = srcModp->clone_ڤوکوق(خطأ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        إذا (AstIface* ثابت newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(true);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(خطأ);

        newModp->name(newname);
        newModp->user2(خطأ);  // We need to re-recurse this module once changed
        newModp->recursive(خطأ);
        newModp->recursiveClone(خطأ);
        newModp->hasGParam(خطأ);
        // Recursion may need level cleanups
        إذا (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        إذا ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_ڤوکوق(), newModp);
            أرجع خطأ;
        }

        إذا (AstClass* ثابت newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_نود* insertp = srcModp;
        طالما (VN_IS(insertp->nextp(), نود_Module)
               && VN_AS(insertp->nextp(), نود_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ثابت auto iter = m_modNameMap.find(newname);
        CloneMap* ثابت clonemapp = &(iter->second.m_cloneMap);
        UINFO(4, "     De-parameterize to new: " << newModp);

        // Grab all I/O so we can remap our pins later
        // Note we allow multiple users of a parameterized model,
        // thus we need to stash this info.
        collectPins(clonemapp, newModp, srcModp->user3p());
        // Relink parameter vars to the new module
        // For interface ports (e.g., l3_if #(W, L0A_W) l3), the parameter pins may
        // reference variables from the enclosing module rather than from the interface
        // being cloned. In such cases, use relinkPinsByName to match by variable name.
        // Check if any parameter pins reference variables outside the cloned interface.
        // This is O(n) but acceptable since parameter pin lists are typically small (<10 pins).
        منطقي needRelinkByName = خطأ;
        إذا (paramsp) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = true;
                    اكسر;
                }
            }
        }
        إذا (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } وإلا {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        لكل (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ثابت AstIfaceRefDType* ثابت portIrefp = it->first;
            ثابت AstIfaceRefDType* ثابت pinIrefp = it->second;
            AstIfaceRefDType* ثابت cloneIrefp = portIrefp->clonep();
            UINFO(8, "     IfaceOld " << portIrefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, portIrefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->ifaceViaCellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        إذا (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->exprp()) {
                إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
                    Ast_نود* ثابت newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ثابت exprp = VN_CAST(newp, Const);
                    AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                    ثابت منطقي overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    إذا (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_ڤوکوق();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ڤوکوق(خطأ));
                    modvarp->overriddenParam(overridden);
                } وإلا إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    Ast_نود_DType* ثابت dtypep = VN_AS(pinp->exprp(), نود_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    إذا (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_ڤوکوق();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_ڤوکوق(خطأ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        أرجع true;
    }
    ثابت ModInfo* moduleFindOrClone(Ast_نود_Module* srcModp, Ast_نود* ifErrorp, AstPin* paramsp,
                                     ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        إذا (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } وإلا {
            إذا (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                أرجع nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ثابت ModInfo* ثابت modInfop = &(it->second);
        أرجع modInfop;
    }

    فراغ convertToStringp(Ast_نود* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        إذا (constp && !constp->اڠک().isString()) {
            constp->replaceWith(
                جديد AstConst{constp->fileline(), AstConst::String{}, constp->اڠک().toString()});
            VL_DO_DANGLING(constp->delete_ڤوکوق(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    فراغ constify_Member_نيلاي(AstVar* varp, std::set<AstVar*>& inProgress) {
        إذا (!varp->valuep() || VN_IS(varp->valuep(), Const)) أرجع;
        إذا (!inProgress.سيسيڤ(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_ڤوکوق();
            varp->valuep(جديد AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            أرجع;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ثابت AstVarRef* refp) {
            AstVar* ثابت refVarp = refp->varp();
            إذا (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.سيسيڤ(refVarp);
            }
        });
        لكل (AstVar* ثابت sibp : siblings) constify_Member_نيلاي(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ساكن AstParseRef* memberParseRef(Ast_نود* nodep) {
        إذا (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) أرجع refp;
        Ast_نود_Pre_Sel* ثابت selp = VN_CAST(nodep, نود_Pre_Sel);
        أرجع selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ساكن فراغ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_نود_اوڠکاڤن* valuep) {
        إذا (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } وإلا {
            Ast_نود_Pre_Sel* ثابت selp = VN_AS(dotp->rhsp(), نود_Pre_Sel);
            إذا (AstAttrOf* ثابت attrp = selp->attrp()) {
                إذا (Ast_نود* ثابت oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ڤوکوق(خطأ));
                    VL_DO_DANGLING(oldFromp->delete_ڤوکوق(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ڤوکوق(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_ڤوکوق(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ساكن منطقي isDeferredMemberBase(ثابت Ast_نود_اوڠکاڤن* nodep) {
        إذا (VN_IS(nodep, MemberSel)) أرجع true;
        إذا (ثابت Ast_نود_Pre_Sel* ثابت selp = VN_CAST(nodep, نود_Pre_Sel)) {
            ثابت Ast_نود_DType* ثابت fromDTypep = selp->fromp()->dtypep();
            ثابت Ast_نود_Array_DType* ثابت arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), نود_Array_DType) : nullptr;
            إذا (arrayDTypep) {
                ثابت Ast_نود_DType* ثابت elemDTypep = arrayDTypep->subDTypep();
                أرجع elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), نود_UOr_Struct_DType);
            }
            أرجع isDeferredMemberBase(selp->fromp());
        }
        ثابت Ast_نود_DType* ثابت dtypep = nodep->dtypep();
        أرجع dtypep && VN_IS(dtypep->skipRefOrNullp(), نود_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ساكن فراغ resolveMemberDot(AstDot* dotp) {
        Ast_نود_اوڠکاڤن* ثابت lhsp = VN_CAST(dotp->lhsp(), نود_اوڠکاڤن);
        AstParseRef* ثابت memberRefp = memberParseRef(dotp->rhsp());
        إذا (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) أرجع;
        AstMemberSel* ثابت newp = جديد AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    فراغ resolveDotToTypedef(Ast_نود* exprp) {
        AstDot* ثابت dotp = VN_CAST(exprp, Dot);
        إذا (!dotp) أرجع;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ثابت classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        إذا (!classRefp) {
            resolveMemberDot(dotp);
            أرجع;
        }
        AstParseRef* ثابت parseRefp = memberParseRef(dotp->rhsp());
        إذا (!parseRefp) أرجع;

        ثابت AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ثابت paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        إذا (paramTypep) {
            Ast_نود_DType* ثابت dtypep = paramTypep->subDTypep();
            AstClassRefDType* ثابت classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            إذا (classRefDTypep) {
                AstClass* ثابت srcClassp = classRefDTypep->classp();
                إذا (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    إذا (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        إذا (classRefp->paramsp()) {
            AstClass* ثابت srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            إذا (srcClassp && srcClassp->hasGParam()) {
                إذا (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        إذا (AstClassRefDType* ثابت crp = class_Ref_DType_Of_نود(classRefp->classOrPackageNodep())) {
            إذا (AstClass* ثابت srcClassp = crp->classp()) {
                إذا (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    إذا (AstClass* ثابت newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        إذا (!lhsClassp) أرجع;

        Ast_نود* ثابت memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        إذا (AstTypedef* ثابت tdefp = VN_CAST(memberp, Typedef)) {
            إذا (parseRefp != dotp->rhsp()) أرجع;
            AstRefDType* ثابت refp = جديد AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ڤوکوق(), dotp);
        } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    فراغ substituteParamMember(AstDot* ثابت dotp, AstParseRef* ثابت memberRefp,
                               AstVar* ثابت varp) {
        إذا (!varp->isParam() || !varp->valuep()) أرجع;
        إذا (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_نيلاي(varp, inProgress);
        }
        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
        إذا (!constp) أرجع;
        AstConst* ثابت newp = constp->clone_ڤوکوق(خطأ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    فراغ resolveParamClassRefDType(AstRefDType* refp) {
        إذا (refp->typedefp() || refp->refDTypep()) أرجع;

        AstClassOrPackageRef* ثابت classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        إذا (!classRefp) أرجع;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        إذا (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            أرجع;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_نود* targetp = classRefp->classOrPackageNodep();
        طالما (ثابت AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        إذا (Ast_نود_DType* ثابت dtargetp = VN_CAST(targetp, نود_DType))
            targetp = dtargetp->skipRefOrNullp();
        إذا (!targetp) أرجع;
        AstClassRefDType* ثابت classRefDTypep = VN_CAST(targetp, ClassRefDType);
        إذا (!classRefDTypep) أرجع;
        srcClassp = classRefDTypep->classp();
        إذا (!srcClassp) أرجع;

        AstClass* newClassp = srcClassp;
        إذا (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ثابت typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        إذا (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ساكن منطقي defaultParamsHaveTypeQueryOnParamType(ثابت AstClassRefDType* nodep) {
        منطقي found = خطأ;
        nodep->foreach([&](ثابت AstAttrOf* attrp) {
            إذا (found || !attrp->attrType().isTypeQuery()) أرجع;
            ثابت AstRefDType* ثابت refp = VN_CAST(attrp->fromp(), RefDType);
            إذا (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = true;
        });
        أرجع found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    منطقي classTypeMatchesDefaultClone(ثابت Ast_نود_DType* exprp, ثابت Ast_نود_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ثابت auto* ثابت اوڠکاڤن_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ثابت auto* ثابت origClassRefp = VN_CAST(origp, ClassRefDType);
        إذا (!اوڠکاڤن_Class_Refp || !origClassRefp) أرجع خطأ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ثابت Ast_نود_Module* ثابت defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        إذا (defaultClonep && defaultClonep == اوڠکاڤن_Class_Refp->classp()) أرجع true;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        إذا (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) أرجع خطأ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        إذا (!origClassRefp->classp()->hasGParam()) أرجع خطأ;
        إذا (origClassRefp->classp()->origName() != اوڠکاڤن_Class_Refp->classp()->origName())
            أرجع خطأ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        إذا (!m_defaultCloneInProgress.سيسيڤ(origClassRefp->classp()).second) أرجع خطأ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ثابت origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ڤوکوق(خطأ));
        Ast_نود_Module* ثابت resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ثابت منطقي match = resolvedModp && VN_CAST(resolvedModp, Class) == اوڠکاڤن_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ڤوکوق(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        أرجع match;
    }

    ساكن منطقي paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        إذا (exprp->اڠک().width() == origp->اڠک().width()) أرجع خطأ;
        إذا (exprp->اڠک().isOpaque()) أرجع خطأ;
        إذا (origp->اڠک().isOpaque()) أرجع خطأ;
        ثابت صحيح maxWidth = std::max(exprp->اڠک().width(), origp->اڠک().width());
        V3_نومبور اوڠکاڤن_اڠک{exprp, maxWidth};
        إذا (exprp->اڠک().isSigned()) {
            اوڠکاڤن_اڠک.opExtendS(exprp->اڠک(), exprp->اڠک().width());
        } وإلا {
            اوڠکاڤن_اڠک.opAssign(exprp->اڠک());
        }
        V3_نومبور orig_اڠک{origp, maxWidth};
        إذا (origp->اڠک().isSigned()) {
            orig_اڠک.opExtendS(origp->اڠک(), origp->اڠک().width());
        } وإلا {
            orig_اڠک.opAssign(origp->اڠک());
        }
        V3_نومبور isEq{exprp, 1};
        isEq.opEq(اوڠکاڤن_اڠک, orig_اڠک);
        أرجع isEq.isNeqZero();
    }

    // Rewrite placeholder __VGIfaceParam pins emitted by
    // LinkDotResolveVisitor::addImplicitParametersOfGenericIface for a nested
    // generic-interface forwarding chain (#7454). Must run here (not in
    // V3LinkDot's Param/Resolve visitor at LDS_PARAMED) because:
    //   * linkDotParamed runs AFTER V3Param, at which point cellDeparam has
    //     already consumed the pin via moduleFindOrClone (variant naming off
    //     pinp->exprp()'s IfaceRefDType) and genericInterfaceVarSetup (hard
    //     VN_AS cast to IfaceRefDType); and
    //   * the enclosing module's port only becomes a concrete IfaceRefDType
    //     after its own nodeDeparam has specialized it. That specialization
    //     happens in V3Param's top-down walk, so the window between "outer
    //     port resolved" and "inner cellDeparam starts" exists only here.
    فراغ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (!pinp->exprp()) استمر;
            إذا (!VString::startsWith(pinp->name(), "__VGIfaceParam")) استمر;
            ثابت Ast_نود_Var_Ref* ثابت fwdRefp = VN_CAST(pinp->exprp(), نود_Var_Ref);
            إذا (!fwdRefp) استمر;
            ثابت AstIfaceRefDType* ثابت resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ثابت newIrefp = جديد AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            إذا (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_ڤوکوق(true));
            }
            pinp->exprp()->unlinkFrBack()->delete_ڤوکوق();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    فراغ name_By_Pin_نيلاي(AstPin* pinp, Ast_نود_Module* srcModp, AstVar* modvarp, string& longnamer,
                        منطقي& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_نيلاي_نومبور(pinp->exprp());
        any_overridesr = true;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    فراغ cellPinCleanup(Ast_نود* nodep, AstPin* pinp, AstPin* paramsp, Ast_نود_Module* srcModp,
                        string& longnamer, منطقي& any_overridesr,
                        std::unordered_set<ثابت AstPin*>& overridingTypePinsr) {
        إذا (!pinp->exprp()) أرجع;  // No-connect
        إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            إذا (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_نيلاي(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } وإلا {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                إذا (is_Aggregate_Param_نيلاي(pinp->exprp())) {
                    name_By_Pin_نيلاي(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    أرجع;
                }
                // Cast/CastSize default values are not yet folded by V3Width.
                // Constify here so the comparison below sees a Const node.
                // Other node kinds are handled in the branches above.
                //
                // Only fold when the cast is self-contained.  modvarp is the
                // parameter on the shared module template and constifyParamsEdit
                // is destructive: a default that reads another parameter
                // (VarRef) or a type parameter (RefDType) must be evaluated per
                // instance, and folding it here would evaluate it against the
                // template's own defaults and bake that in for every later
                // instance that relies on the default.
                إذا (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    ثابت منطقي dependent = modvarp->valuep()->exists(
                        [](Ast_نود* np) { أرجع VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    إذا (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                إذا (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ثابت exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                إذا (exprp && !exprp->اڠک().isDouble() && !exprp->اڠک().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ڤوکوق(خطأ);
                    منطقي cloneVarpUnresolved = خطأ;
                    إذا (Ast_نود* ثابت oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_ڤوکوق(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ڤوکوق(خطأ));
                    إذا (Ast_نود_DType* ثابت origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        إذا (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_ڤوکوق();
                        cloneVarp->childDTypep(origDTypep->clone_ڤوکوق(خطأ));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr صحيح maxSubstIters = 1000;
                        لكل (صحيح it = 0; it < maxSubstIters; ++it) {
                            منطقي any = خطأ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ثابت targetp = varrefp->varp();
                                Ast_نود* replacep = nullptr;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        إذا (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ڤوکوق(خطأ);
                                        }
                                        اكسر;
                                    }
                                }
                                إذا (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ڤوکوق(خطأ);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ثابت patp = VN_CAST(replacep, Pattern);
                                إذا (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_ڤوکوق(خطأ));
                                }
                                إذا (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ڤوکوق(), varrefp);
                                    any = true;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_نود_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ثابت ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                إذا (!ptdp) أرجع;
                                AstPin* overridePinp = nullptr;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        اكسر;
                                    }
                                }
                                Ast_نود_DType* ثابت substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), نود_DType)
                                                   : ptdp->subDTypep();
                                إذا (substp) toReplace.emplace_back(refp, substp);
                            });
                            لكل (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ثابت refp = it->first;
                                refp->replaceWith(it->second->clone_ڤوکوق(خطأ));
                                VL_DO_DANGLING(refp->delete_ڤوکوق(), refp);
                                any = true;
                            }
                            إذا (!any) اكسر;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_نود* np) {
                            إذا (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = true;
                                أرجع;
                            }
                            إذا (AstEnumItemRef* ثابت itemRefp = VN_CAST(np, EnumItemRef)) {
                                إذا (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = true;
                                }
                                أرجع;
                            }
                            AstRefDType* ثابت refp = VN_CAST(np, RefDType);
                            إذا (!refp) أرجع;
                            إذا (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = true;
                            }
                            // A type declared in the template must not be widthed there
                            إذا (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = true;
                            }
                        });
                    }
                    إذا (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        إذا (AstConst* ثابت widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            إذا (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ڤوکوق(), cloneVarp);
                }
                AstConst* ثابت namingExprp = normedNamep ? normedNamep : exprp;
                إذا (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_نود* ثابت pinExprp = pinp->exprp();
                    pinExprp->replaceWith(جديد AstConst{pinp->fileline(), AstConst::Widthed_نيلاي{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ڤوکوق(), pinExprp);
                } وإلا إذا (origp
                           && (exprp->same_ڤوکوق(origp)
                               || (exprp->اڠک().width() == origp->اڠک().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } وإلا إذا (namingExprp->اڠک().isDouble() || namingExprp->اڠک().isString()
                           || namingExprp->اڠک().isFourState()
                           || namingExprp->اڠک().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_نيلاي_نومبور(namingExprp));
                    any_overridesr = true;
                } وإلا {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->اڠک().ascii(خطأ));
                    any_overridesr = true;
                }
                إذا (normedNamep) VL_DO_DANGLING(normedNamep->delete_ڤوکوق(), normedNamep);
            }
        } وإلا إذا (AstParamTypeDType* ثابت modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_نود_DType* rawTypep = VN_CAST(pinp->exprp(), نود_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            منطقي skipWidthForTemplateStruct = خطأ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_نود_DType* ثابت resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                إذا (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_نود_Module* ثابت ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    إذا (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = true;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                إذا (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_نود_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ثابت Ast_نود_DType* origp = modvarp->skipRefToNonRefp();
            إذا (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } وإلا {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                إذا (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), نود_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                إذا (!modvarp->fwdType().is_نود_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                إذا (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } وإلا {
                    إذا (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), نود_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    إذا (AstClassRefDType* ثابت classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        إذا (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } وإلا {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    فراغ cellInterfaceCleanup(AstPin* pinsp, Ast_نود_Module* srcModp, string& longnamer,
                              منطقي& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        لكل (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ثابت AstVar* ثابت modvarp = pinp->modVarp();
            إذا (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                استمر;
            }
            إذا (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ثابت portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ثابت Ast_نود* ثابت exprp = pinp->exprp();
                إذا (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    استمر;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                إذا (ثابت Ast_نود_Var_Ref* ثابت vrp = VN_CAST(exprp, نود_Var_Ref)) {
                    إذا (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                إذا (!pinIrefp && exprp) {
                    ثابت Ast_نود* basep = exprp;
                    طالما (ثابت Ast_نود_Pre_Sel* ثابت selp = VN_CAST(basep, نود_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(basep, VarRef)) {
                        إذا (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                إذا (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } وإلا إذا (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } وإلا {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    إذا (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_نيلاي_نومبور(pinIrefp));
                        any_overridesr = true;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        إذا (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_نود::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_نود::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    فراغ storeOriginalParams(Ast_نود_Module* ثابت classp) {
        لكل (Ast_نود* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_نود* originalParamp = nullptr;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam()) originalParamp = varp->clonep();
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    فراغ genericInterfaceVarSetup(ثابت AstPin* ثابت paramsp, ثابت AstPin* ثابت pinsp) {
        std::unordered_map<string, ثابت AstPin*> paramspMap;
        لكل (ثابت AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.سيسيڤ({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        إذا (paramspMap.empty()) أرجع;

        لكل (ثابت Ast_نود* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (AstVar* ثابت varp = pinp->modVarp()) {
                    Ast_نود_DType* dtypep = varp->childDTypep()->elemDTypep();
                    إذا (AstIfaceGenericDType* ثابت ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ثابت auto iter = paramspMap.find(varp->name());
                        إذا (iter == paramspMap.end()) استمر;
                        Ast_نود* ثابت backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ثابت AstPin* ثابت paramp = iter->second;
                        paramspMap.erase(iter);
                        ثابت AstIfaceRefDType* ثابت ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ثابت newIfacerefp = جديد AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        إذا (auto* ثابت arrDtp = VN_CAST(backp, نود_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } وإلا إذا (auto* ثابت arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } وإلا {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.تولق_Deletep(ifaceGDTypep), ifaceGDTypep);
                        إذا (paramspMap.empty()) أرجع;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    فراغ resolveDefaultParams(Ast_نود* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        إذا (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            إذا (!classp) أرجع;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } وإلا إذا (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } وإلا {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        لكل (Ast_نود* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                إذا (!paramTypep->isGParam()) استمر;
                m_paramIndex.emplace(paramTypep, static_cast<صحيح>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_نود_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        لكل (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* ثابت paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ثابت reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            إذا (!reqDtp) استمر;

            // If default is a reference to another param type, record dependency
            إذا (AstRefDType* ثابت refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                إذا (AstParamTypeDType* ثابت sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    إذا (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    استمر;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            إذا (Ast_نود_DType* ثابت dtp = VN_CAST(reqDtp->lhsp(), نود_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ثابت typep = pinp->modPTypep();
            ثابت auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            إذا (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } وإلا {
                pinsByIndex.تولق_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        لكل (منطقي resolvedSome = true; resolvedSome;) {
            resolvedSome = خطأ;
            لكل (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                إذا (pinsByIndex[paramIdx]) استمر;
                ثابت صحيح sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                إذا (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_ڤوکوق(خطأ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                إذا (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_نود_DType* ثابت dtypep = defaultTypeNodes[paramIdx];
                    newPinp = جديد AstPin{dtypep->fileline(), static_cast<صحيح>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_ڤوکوق(خطأ)};
                }

                إذا (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(true);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    إذا (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } وإلا إذا (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    إذا (!paramsp) paramsp = newPinp;
                    resolvedSome = true;
                }
            }
        }
    }

    Ast_نود_Module* نود_Deparam_Common(Ast_نود* nodep, Ast_نود_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, منطقي any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        إذا (debug() >= 9 && paramsp) paramsp->dump_ڤوکوق_And_Next(cout, "-  cellparams: ");

        إذا (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } وإلا {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ثابت AstPin*> overridingTypePins;
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;  // An empty override keeps the default
                إذا (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    استمر;
                }
                AstVar* ثابت modvarp = pinp->modVarp();
                إذا (!modvarp || !modvarp->isGParam()) استمر;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ثابت AstPattern* ثابت patternp = VN_CAST(pinp->exprp(), Pattern);
                إذا (patternp && !patternp->childDTypep()) استمر;
                إذا (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_نيلاي(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    إذا (!overridingTypePins.کيراءن(pinp)) استمر;
                    Ast_نود_DType* ثابت typep = VN_AS(pinp->exprp(), نود_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_نيلاي_نومبور(typep->skipRefToNonRefp());
                    any_overrides = true;
                    استمر;
                }
                cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                               any_overrides /*ref*/, overridingTypePins /*ref*/);
            }
        }
        IfaceRefRefs ifaceRefRefs;
        cellInterfaceCleanup(pinsp, srcModp, longname /*ref*/, any_overrides /*ref*/,
                             ifaceRefRefs /*ref*/);

        // Template classes with type parameters need specialization even when types match
        // defaults. This is required for UVM parameterized classes. However, interfaces should NOT
        // be specialized when type params match defaults (needed for nested interface ports).
        // Already-specialized clones (hasGParam=false) must not be re-cloned, otherwise
        // nested class type parameters cause unbounded re-deparameterization (Holder_ ->
        // Holder__).
        منطقي defaultsResolved = خطأ;
        إذا (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modPTypep()) {
                    any_overrides = true;
                    defaultsResolved = true;
                    اكسر;
                }
            }
        }
        إذا (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = true;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_نود_Module* newModp = nullptr;
        إذا (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_نود_Module* ثابت paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            إذا (!paramedModp) أرجع nullptr;  // Unsupported, already reported
            paramedModp->dead(خطأ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } وإلا إذا (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            إذا (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_نود_Module* نود_Copyp = srcModp->clone_ڤوکوق(خطأ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << نود_Copyp);
                m_deleter.تولق_Deletep(نود_Copyp);  // nodeCopyp used past here
                srcModp->user3p(نود_Copyp);
                storeOriginalParams(نود_Copyp);
            }
            newModp = srcModp;
        } وإلا {
            ثابت string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ثابت ModInfo* ثابت modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            إذا (!modInfop) أرجع nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ثابت منطقي cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ثابت newClassp = VN_CAST(newModp, Class);
        إذا (newClassp && !newClassp->user2()) m_specializedClassps.تولق_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        إذا (defaultsResolved) srcModp->user4p(newModp);

        لكل (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        إذا (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_ڤوکوق(), paramsp);
        أرجع newModp;
    }

    Ast_نود_Module* cellDeparam(AstCell* nodep, Ast_نود_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_نود_Module* ثابت newModp = نود_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        إذا (!newModp) أرجع nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(خطأ);
        أرجع newModp;
    }
    Ast_نود_Module* ifaceRefDeparam(AstIfaceRefDType* ثابت nodep, Ast_نود_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        إذا (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ثابت string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ثابت string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ثابت size_t ownerPos = ownerBaseName.find("__");
            إذا (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ثابت size_t srcPos = srcBaseName.find("__");
            إذا (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            إذا (ownerBaseName == srcBaseName) {
                منطقي allOwnParams = true;
                لكل (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    إذا (AstRefDType* ثابت refp = VN_CAST(pinp->exprp(), RefDType)) {
                        إذا (AstParamTypeDType* ثابت ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_نود_Module* ثابت ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            إذا (ptdOwnerp != m_modp) allOwnParams = خطأ;
                        } وإلا {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } وإلا {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                إذا (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    إذا (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_ڤوکوق();
                    أرجع ownerIfacep;
                }
            }
        }

        Ast_نود_Module* ثابت newModp
            = نود_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, خطأ);
        إذا (!newModp) أرجع nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        أرجع newModp;
    }
    Ast_نود_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_نود_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_نود_Module* ثابت newModp
            = نود_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, خطأ);
        إذا (!newModp) أرجع nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ثابت refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ثابت newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        إذا (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            إذا (AstTypedef* ثابت typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        أرجع newModp;
    }
    Ast_نود_Module* classRefDeparam(AstClassRefDType* nodep, Ast_نود_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_نود_Module* ثابت newModp
            = نود_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, خطأ);
        إذا (!newModp) أرجع nullptr;
        AstClass* ثابت newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        أرجع newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    صنف DeferredResolverVisitor final : عام VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ثابت Ast_نود*> m_reachedDecls;

        منطقي firstReach(ثابت Ast_نود* ثابت declp) { أرجع m_reachedDecls.سيسيڤ(declp).second; }

        فراغ iterateParamType(AstParamTypeDType* ثابت paramTypep) {
            إذا (firstReach(paramTypep)) iterate(paramTypep);
        }
        فراغ iterateTypedef(AstTypedef* ثابت tdefp) {
            إذا (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }
        فراغ visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_نود* ثابت targetp = nodep->classOrPackageNodep();
            إذا (AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        فراغ visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        فراغ visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ثابت tdefp = nodep->typedefp();
            إذا (tdefp) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        فراغ visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ثابت varp = nodep->varp();
            إذا (firstReach(varp)) {
                iterate(varp->subDTypep());
                ثابت auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                إذا (varp->varType() == VVarType::LPARAM && deferredVarps.کيراءن(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    عام:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_نود* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

عام:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    فراغ retargetIfaceRefs(Ast_نود_Module* parentModp, ثابت string& cellName) {
        Ast_نود_Module* ثابت correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        إذا (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) أرجع;
        parentModp->foreach([&](AstRefDType* refp) {
            ثابت VIfaceCaptureTag* ثابت tagp = V3LinkDotIfaceCapture::captureTag(refp);
            إذا (!tagp || tagp->m_cellPath != cellName) أرجع;
            // Only retarget REFDTYPEs owned by parentModp.
            إذا (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) أرجع;
            إذا (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    فراغ resolveDeferredDotsReachableFrom(Ast_نود* rootp, ثابت Ast_نود_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*هذا, rootp};
    }

    Ast_نود_Module* نود_Deparam(Ast_نود* nodep, Ast_نود_Module* srcModp, Ast_نود_Module* modp,
                               ثابت string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_نود::نود_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string نود_Name = nodep->name();
        إذا (AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (نود_Name.empty()) نود_Name = ifaceRefp->cellName();
        }
        ثابت string instanceName
            = نود_Name.empty() ? someInstanceName : (someInstanceName + "." + نود_Name);

        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            لكل (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ثابت nextp = VN_AS(pinp->nextp(), Pin);
                إذا (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_ڤوکوق(), pinp);
                }
                pinp = nextp;
            }
            // Nested generic-iface forwarding (#7454): rewrite VarRef placeholders
            // left by V3LinkDot to concrete IfaceRefDTypes now that the enclosing
            // module has been specialized.
            resolveGenericIfaceForwardingPins(cellp->paramsp());
        }
        // Create new module name with _'s between the constants
        UINFOTREE(10, nodep, "", "cell");
        // Resolve `class::member` Dots in pin values, and in any deferred
        // lparam reachable from the pin tree, so constify sees Consts.
        resolveDeferredDotsReachableFrom(nodep, modp);
        // Evaluate all module constants
        V3Const::constifyParamsEdit(nodep);
        // Set name for warnings for when we param propagate the module
        // For AstIfaceRefDType, name() returns the modport name (often empty),
        // so use cellName() which is the actual cell instance name.
        // If both are empty (interface port, not a cell), skip appending
        // to avoid double-dots in the path.
        srcModp->someInstanceName(instanceName);
        UINFO(9, "nodeDeparam SET-SRC-INST srcMod="
                     << srcModp->prettyNameQ() << " someInstanceName='"
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_نود::نود_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_نود_Module* newModp = nullptr;
        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } وإلا إذا (AstIfaceRefDType* ثابت ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } وإلا إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        إذا (!newModp) أرجع nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_نود::نود_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        أرجع newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        أرجع taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        لكل (Ast_نود_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), نود_Module)) {
            m_allModuleNames.سيسيڤ(modp->name());
        }
    }
    ~ParamProcessor() = افتراضي;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

صنف ClassRefUnlinkerVisitor final : عام VNVisitor {

عام:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        إذا (nodep->paramsp()) {
            إذا (AstClass* ثابت classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                إذا (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ثابت dotp = VN_CAST(nodep->backp(), Dot);
                    إذا (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(تولق_Deletep(dotp), dotp, nodep);
                    } وإلا {
                        VL_DO_DANGLING(تولق_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    فراغ visit(AstClass* nodep) override {}  // don't iterate inside classes
    فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

صنف ParamClassRefDTypeRelinkVisitor final : عام VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ثابت AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_نود_Module* m_ownerModp = nullptr;
    Ast_نود_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    فراغ ensureOwnerMap() {
        إذا (m_ownerMapForp == m_ownerModp) أرجع;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        إذا (!m_ownerModp) أرجع;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (ownerClassp->hasGParam()) أرجع;
        }
        لكل (Ast_نود* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ثابت tdp = VN_CAST(sp, Typedef);
            إذا (!tdp) استمر;
            AstClassRefDType* ثابت crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            إذا (!crdtp || !crdtp->classp()) استمر;
            AstClass* ثابت targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            إذا (targetp->hasGParam()) استمر;
            ثابت std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ثابت auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            إذا (!pair.second && pair.first->second != targetp) m_ambiguous.سيسيڤ(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ساكن string classOrigName(ثابت AstClass* classp) {
        أرجع classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ساكن AstTypedef* findTypedefWithBases(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            إذا (AstTypedef* ثابت tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                إذا (tdp->subDTypep()) أرجع tdp;
            }
        }
        أرجع nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    فراغ retargetRefDType(AstRefDType* refp) {
        إذا (!m_ownerModp) أرجع;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ثابت AstClass* refClassp = nullptr;
                إذا (AstTypedef* ثابت tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } وإلا إذا (ثابت AstClassOrPackageRef* ثابت classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                إذا (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ثابت selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    إذا (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        أرجع;
                    }
                }
            }
        }

        AstTypedef* ثابت oldTdp = refp->typedefp();
        إذا (!oldTdp) أرجع;
        AstClass* ثابت oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        إذا (!oldOwnerp) أرجع;

        ensureOwnerMap();
        إذا (m_origNameToClone.empty()) أرجع;
        ثابت std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        إذا (m_ambiguous.کيراءن(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            أرجع;
        }
        ثابت auto it = m_origNameToClone.find(origName);
        إذا (it == m_origNameToClone.end()) أرجع;
        AstClass* ثابت correctClassp = it->second;
        إذا (correctClassp == oldOwnerp) أرجع;
        AstTypedef* ثابت newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        إذا (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            أرجع;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        إذا (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            أرجع;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

عام:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ visit(Ast_نود_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        لكل (Ast_نود* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        إذا (AstParamTypeDType* ثابت paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ثابت auto it = m_paramTypeClassMap.find(paramtypep);
            إذا (it != m_paramTypeClassMap.end()) {
                AstClass* ثابت origClassp = it->second;
                إذا (origClassp->hasGParam() && !origClassp->user3p()) {
                    إذا (AstClass* ثابت parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ثابت string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        لكل (Ast_نود* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                إذا (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    اكسر;
                                }
                            }
                        }
                        إذا (newParamTypep) {
                            nodep->refDTypep(newParamTypep);
                            nodep->classOrPackagep(parametrizedClassp);
                        }
                    }
                }
            }
        }
        // Retarget REFDTYPE.typedefp using the current owner's own
        // resolved typedefs as ground truth (handles sibling-clone stomping).
        retargetRefDType(nodep);
    }
    فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

صنف ParamState final {
عام:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<منطقي, صحيح>;
    std::multimap<WQKey, Ast_نود_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_نود_Module*, std::unordered_set<Ast_نود_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

صنف ParamVisitor final : عام VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    منطقي m_iterateModule = خطأ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<منطقي, Ast_نود*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ثابت Ast_نود*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ثابت AstRefDType*, منطقي>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ثابت Ast_نود_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_نود_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    منطقي m_modIfaceRefsDone = خطأ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    منطقي m_inGenerateCond = خطأ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    فراغ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_نود_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = true;

        // Visit all cells under module, recursively
        طالما (true) {
            // Classes specialized since the last pass still need their bodies elaborated
            لكل (AstClass* ثابت classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{true, classp->level()}, classp);
            }
            إذا (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            إذا (workQueue.empty()) اكسر;

            ثابت auto itm = workQueue.cbegin();
            Ast_نود_Module* ثابت modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            إذا (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                إذا (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = خطأ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            طالما (!m_cellps.empty()) {
                ثابت auto itim = m_cellps.cbegin();
                Ast_نود* ثابت cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ثابت auto genHierIt = m_genHierNames.find(cellp);
                إذا (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_نود_Module* srcModp = nullptr;
                إذا (ثابت AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } وإلا إذا (ثابت AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    إذا (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) استمر;
                } وإلا إذا (ثابت AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } وإلا إذا (ثابت AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } وإلا {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                إذا (!srcModp) استمر;

                // Update path
                ثابت string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                إذا (Ast_نود_Module* ثابت newModp
                    = m_processor.نود_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    إذا (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        إذا (V3LinkDotIfaceCapture::enabled()) {
                            إذا (ثابت AstCell* ثابت modCellp = VN_CAST(cellp, Cell)) {
                                إذا (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].سيسيڤ(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    إذا (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = خطأ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ساكن string getRefBaseName(ثابت AstVarXRef* refp) {
        ثابت string dotted = refp->dotted();
        إذا (dotted.empty()) أرجع "";
        أرجع dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    فراغ logTemplateLeakRefs(Ast_نود_Module* parentModp, Ast_نود_Module* templateModp,
                             ثابت حرف* stage, Ast_نود* contextp) {
        إذا (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) أرجع;
        // LCOV_EXCL_START  // Debug-only diagnostic
        صحيح leak_کيراءن = 0;
        ثابت auto ancestryOf = [](ثابت Ast_نود* nodep) {
            string ancestry;
            لكل (ثابت Ast_نود* curp = nodep; curp; curp = curp->backp()) {
                إذا (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                إذا (VN_IS(curp, نود_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    اكسر;
                }
            }
            أرجع ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            إذا (refp->typedefp()) {
                Ast_نود_Module* ثابت tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                إذا (tdOwnerp == templateModp) {
                    ++leak_کيراءن;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_نود::نود_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            إذا (refp->refDTypep()) {
                Ast_نود_Module* ثابت rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                إذا (rdOwnerp == templateModp) {
                    ++leak_کيراءن;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_نود::نود_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            إذا (!varrefp->varp()) أرجع;
            Ast_نود_Module* ثابت varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            إذا (varOwnerp != templateModp) أرجع;
            ++leak_کيراءن;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_نود::نود_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        إذا (leak_کيراءن > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_کيراءن);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    فراغ checkParamNotHierRecurse(Ast_نود* nodep, منطقي underTypeQuery = خطأ) {
        لكل (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ثابت AstAttrOf* ثابت attrp = VN_CAST(nodep, AttrOf);
            ثابت منطقي childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                منطقي isIfaceRef = خطأ;
                إذا (refp->varp() && refp->varp()->isIfaceParam()) {
                    ثابت string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.کيراءن(refname) || m_ifaceInstCells.کيراءن(refname));
                }
                إذا (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } وإلا إذا (ثابت Ast_نود_FTask_Ref* ثابت refp = VN_CAST(nodep, نود_FTask_Ref)) {
                إذا (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op2p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op3p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op4p(), childUnderQuery);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    فراغ specializeNestedIfaceCells(Ast_نود_Module* ifaceModp) {
        لكل (Ast_نود* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت nestedCellp = VN_CAST(stmtp, Cell);
            إذا (!nestedCellp) استمر;
            إذا (!VN_IS(nestedCellp->modp(), Iface)) استمر;
            إذا (!nestedCellp->paramsp()) استمر;

            Ast_نود_Module* ثابت nestedSrcModp = nestedCellp->modp();
            إذا (Ast_نود_Module* ثابت nestedNewModp = m_processor.نود_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                إذا (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    لكل (Ast_نود* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        إذا (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                            إذا (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    إذا (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    منطقي cellParamsReferenceIfacePorts(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع خطأ;

        لكل (Ast_نود* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            إذا (AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (Ast_نود* ثابت exprp = pinp->exprp()) {
                    إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
                        ثابت string refname = getRefBaseName(refp);
                        إذا (!refname.empty()
                            && (m_ifacePortNames.کيراءن(refname)
                                || m_ifaceInstCells.کيراءن(refname)))
                            أرجع true;
                    }
                }
            }
        }
        أرجع خطأ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    فراغ expandIfaceArrayPortDimensions(AstVar* portp, Ast_نود_DType* dtypep,
                                        ثابت std::string& suffix) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = portp->clone_ڤوکوق(خطأ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            إذا (Ast_نود_DType* ثابت oldp = varp->childDTypep()) {
                VL_DO_DANGLING(تولق_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_ڤوکوق(خطأ));
            portp->addNextHere(varp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        لكل (ثابت صحيح n : arrp->declRange().seq_کانن_To_کيري()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_نود::encode_نومبور(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    فراغ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ثابت AstRange* rangep, ثابت std::string& suffix, صحيح idx) {
        // Base case: insert the element when no dimensions left
        إذا (!rangep) {
            AstCell* ثابت elemp = arrayedCellp->clone_ڤوکوق(خطأ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            إذا (ifaceVarp) {
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ڤوکوق(خطأ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ثابت varp = ifaceVarp->clone_ڤوکوق(خطأ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                إذا (Ast_نود_DType* ثابت oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(تولق_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            أرجع;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);
        لكل (ثابت صحيح n : rangep->seq_کانن_To_کيري()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_نود::encode_نومبور(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    فراغ visitCellOrClassRef(Ast_نود* nodep, منطقي isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        إذا (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ثابت cellp = VN_CAST(nodep, Cell);
            إذا (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_نود_Module* ثابت srcModp = cellp->modp();
                Ast_نود_Module* ثابت newModp
                    = m_processor.نود_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                إذا (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                إذا (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        إذا (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    فراغ visit(Ast_نود_Module* nodep) override {
        إذا (nodep->recursiveClone()) nodep->dead(true);  // Fake, made for recursive elimination
        إذا (nodep->dead()) أرجع;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            إذا (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.تولق_back(classp);
                أرجع;
            }
        }

        إذا (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            أرجع;
        }

        // Start traversal at root-like things
        إذا (nodep->is_اتس()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    منطقي isCircularType(ثابت AstRefDType* nodep) {
        ثابت auto iter = m_isCircular.emplace(nodep, true);
        إذا (!iter.second) أرجع iter.first->second;
        إذا (ثابت AstRefDType* ثابت subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ثابت منطقي ret = isCircularType(subDTypep);
            iter.first->second = ret;
            أرجع ret;
        }
        iter.first->second = خطأ;
        أرجع خطأ;
    }

    فراغ visit(AstRefDType* nodep) override {
        ثابت منطقي isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        إذا (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        إذا (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } وإلا إذا (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2SetOnce()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstCell* nodep) override {
        إذا (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            لكل (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ثابت ifaceVarp = [&]() -> AstVar* {
                إذا (!VN_IS(nodep->modp(), Iface)) أرجع nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ثابت varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                أرجع varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            إذا (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(true);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ثابت firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(تولق_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            أرجع;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        إذا (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        إذا (nodep->ifacep()) visitCellOrClassRef(nodep, true);
    }
    فراغ visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, خطأ);
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        إذا (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, خطأ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ساكن فراغ gatherVars(Ast_نود* stmtsp, منطقي (*matchp)(ثابت AstVar*), VarsByName& varsr) {
        لكل (Ast_نود* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                إذا (matchp(varp)) varsr.emplace(varp->name(), varp);
            } وإلا إذا (AstGenBlock* ثابت genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ثابت VarsByName& modIfaceRefs() {
        إذا (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = true;
        }
        أرجع m_modIfaceRefs;
    }
    ثابت VarsByName& ifaceParams(Ast_نود_Module* ifacep) {
        ثابت auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        إذا (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isParam(); },
                pair.first->second);
        }
        أرجع pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_نود* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ثابت lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ثابت rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        إذا (!lhsp || !rhsp) أرجع nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        إذا (VL_UNCOVERABLE(!m_modp)) أرجع nullptr;

        ثابت auto& refVars = modIfaceRefs();
        ثابت auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        إذا (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) أرجع nullptr;
        AstVar* ثابت ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ثابت ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        إذا (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) أرجع nullptr;

        ثابت auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ثابت auto targetVarIt = ifaceVars.find(rhsp->name());
        إذا (targetVarIt == ifaceVars.end()) أرجع nullptr;
        AstVar* ثابت targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        إذا (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        إذا (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            أرجع nullptr;  // LCOV_EXCL_LINE
        }
        أرجع targetVarp->valuep()->clone_ڤوکوق(خطأ);
    }
    فراغ resolveGenericIfaceDotsIn(Ast_نود* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.تولق_back(dotp); });
        لكل (AstDot* ثابت dotp : dotps) {
            إذا (Ast_نود* ثابت newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ڤوکوق(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    فراغ visit(AstVar* nodep) override {
        إذا (nodep->user2SetOnce()) أرجع;  // Process once
        // Interface array port: split it into its elements
        إذا (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(true);
        }
        // Build cache of interface port names as we encounter them
        إذا (nodep->isIfaceRef()) { m_ifacePortNames.سيسيڤ(nodep->name()); }
        iterateChildren(nodep);
        إذا (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            إذا (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } وإلا إذا (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                منطقي hasUnresolvedLparamXRef = خطأ;
                nodep->valuep()->foreach([&](ثابت AstVarXRef* xrefp) {
                    إذا (ثابت AstVar* ثابت varp = xrefp->varp()) {
                        إذا (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = true;
                        }
                    }
                });
                إذا (hasUnresolvedLparamXRef) أرجع;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ثابت منطقي hasDot = nodep->valuep()->exists([](AstDot*) { أرجع true; });
                منطقي refsDeferred = خطأ;
                إذا (!hasDot) {
                    ثابت auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ثابت AstVarRef* refp) {
                        إذا (refsDeferred) أرجع;
                        AstVar* ثابت refVarp = refp->varp();
                        إذا (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.کيراءن(refVarp)) {
                            refsDeferred = true;
                        }
                    });
                }
                إذا (hasDot || refsDeferred) {
                    v3Global.rootp()->تولق_Deferred_Param_Varp(nodep);
                    أرجع;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    فراغ visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        إذا (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    فراغ visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        إذا (nodep->varp()) iterate(nodep->varp());
    }
    منطقي ifaceParamReplace(AstVarXRef* nodep, Ast_نود* candp) {
        لكل (; candp; candp = candp->nextp()) {
            إذا (nodep->name() == candp->name()) {
                إذا (AstVar* ثابت varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    إذا (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    أرجع true;
                } وإلا إذا (ثابت AstPin* ثابت pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ڤوکوق(خطأ));
                    VL_DO_DANGLING(تولق_Deletep(nodep), nodep);
                    أرجع true;
                }
            }
        }
        أرجع خطأ;
    }

    فراغ deparamIfaceCellNow(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع;
        إذا (!VN_IS(cellp->modp(), Iface)) أرجع;
        Ast_نود_Module* ثابت srcModp = cellp->modp();
        Ast_نود_Module* ثابت newModp
            = m_processor.نود_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        إذا (newModp && newModp != srcModp) {
            إذا (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    فراغ visit(Ast_نود_FTask_Ref* nodep) override {
        إذا (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        إذا (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    فراغ visit(AstVarXRef* nodep) override {
        إذا (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            أرجع;
        }
        // Parameters of an interface array are accessed through its first element
        إذا (nodep->varp() && nodep->varp()->isParam()) {
            ثابت auto it = m_ifaceInstCells.find(nodep->dotted());
            إذا (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ثابت string dotted = nodep->dotted();
        إذا (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ثابت Ast_نود* backp = nodep;
            طالما ((backp = backp->backp())) {
                إذا (VN_IS(backp, نود_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    اكسر;
                }
                إذا (ثابت AstVar* ثابت varp = VN_CAST(backp, Var)) {
                    إذا (!varp->isIfaceRef()) استمر;
                    // Through all dimensions of an array
                    ثابت AstIfaceRefDType* ثابت ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    إذا (!ifacerefp) استمر;
                    // Interfaces passed in on the port map have ifaces
                    إذا (ثابت AstIface* ثابت ifacep = ifacerefp->ifacep()) {
                        إذا (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            إذا (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    وإلا إذا (AstCell* ثابت cellp = ifacerefp->cellp()) {
                        إذا (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            إذا (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ثابت auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            إذا (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ثابت cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    فراغ visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ثابت AstClass* lhsClassp = nullptr;
        ثابت AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        إذا (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_نود* rhsDefp = nullptr;
        AstClassOrPackageRef* ثابت rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        إذا (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        إذا (lhsClassp && rhsDefp) {
            m_state.m_dots.تولق_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } وإلا {
            iterate(nodep->rhsp());
        }
    }

    فراغ visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ثابت varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_نود_FTask_Ref* ثابت taskrefp = VN_CAST(nodep->refp(), نود_FTask_Ref);
        AstEnumItemRef* ثابت enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        إذا (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } وإلا إذا (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } وإلا إذا (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } وإلا {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            أرجع;
        }
        iterate(nodep->cellrefp());

        إذا (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } وإلا إذا (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } وإلا {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(تولق_Deletep(nodep), nodep);
    }
    فراغ visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        لكل (Ast_نود *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<صحيح> indices;
        لكل (Ast_نود* s = nodep->selp(); s; s = s->nextp()) {
            ثابت AstConst* ثابت constp = VN_CAST(s, Const);
            إذا (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                أرجع;
            }
            indices.تولق_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ثابت string viftopSuffix = "__Viftop";
        ثابت string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        ثابت string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        إذا (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            أرجع;
        }
        pos += baseName.length();
        لكل (صحيح idx : indices) {
            ثابت string replacement = "__BRA__" + Ast_نود::encode_نومبور(idx) + "__KET__";
            إذا (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                أرجع;
            }
            m_unlinkedTxt.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    فراغ iterateGenerateCond(Ast_نود* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = true;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    فراغ visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
            إذا (Ast_نود* ثابت keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } وإلا {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_ڤوکوق(), nodep);
            // Normal edit rules will now recurse the replacement
        } وإلا {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    فراغ visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        إذا (AstGenFor* ثابت forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3Width::widthParamsEdit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ثابت string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            إذا (Ast_نود* ثابت stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } وإلا {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    فراغ visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        منطقي hit = خطأ;
        Ast_نود* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        لكل (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            لكل (Ast_نود* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ثابت AstConst* ثابت exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        لكل (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            لكل (Ast_نود* ep = itemp->condsp(); ep;) {
                Ast_نود* ثابت nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        لكل (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            إذا (!itemp->isDefault()) {
                لكل (Ast_نود* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    إذا (ثابت AstConst* ثابت ccondp = VN_CAST(ep, Const)) {
                        V3_نومبور match{nodep, 1};
                        match.opEq(ccondp->اڠک(), exprp->اڠک());
                        إذا (!hit && match.isNeqZero()) {
                            hit = true;
                            keepp = itemp->itemsp();
                        }
                    } وإلا {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        لكل (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            إذا (itemp->isDefault()) {
                إذا (!hit) {
                    hit = true;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        إذا (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ڤوکوق(), nodep);
    }

    فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = افتراضي;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

صنف Param_اتس final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    فراغ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        لكل (AstDot* ثابت dotp : m_state.m_dots) {
            ثابت AstClassOrPackageRef* ثابت classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ثابت AstClass* ثابت lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            لكل (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    اكسر;
                }
            }
        }
    }

    فراغ fixLevel(Ast_نود_Module* modp) {
        // Fix up level of module, based on who instantiates it
        إذا (modp->user1SetOnce()) أرجع;  // Already fixed
        إذا (m_state.m_parentps[modp].empty()) أرجع;  // Leave top levels alone
        صحيح maxParentLevel = 0;
        لكل (Ast_نود_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        إذا (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    فراغ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_نود_Module*> modps;
        لكل (Ast_نود_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), نود_Module);
            modp->unlinkFrBack();
            modps.تولق_back(modp);
        }

        // Fix-up levels
        ثابت VNUser1InUse user1InUse;
        لكل (Ast_نود_Module* ثابت modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ثابت Ast_نود_Module* ap, ثابت Ast_نود_Module* bp) {
                             أرجع ap->level() < bp->level();
                         });

        // Re-insert modules
        لكل (Ast_نود_Module* ثابت modp : modps) netlistp->addModulesp(modp);
    }

عام:
    // CONSTRUCTORS
    explicit Param_اتس(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ثابت ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ثابت ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_نود_FTask_Ref* ftaskrefp) {
            Ast_نود_FTask* ftaskp = ftaskrefp->taskp();
            إذا (!ftaskp || !ftaskp->classMethod()) أرجع;
            ثابت std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ثابت auto ancestorClassOf = [](Ast_نود* startp) -> AstClass* {
                لكل (Ast_نود* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    إذا (AstClass* ثابت cp = VN_CAST(np, Class)) أرجع cp;
                }
                أرجع nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            إذا (refClassp == ftaskrefp->classOrPackagep())
                أرجع;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            إذا (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                أرجع;
            }
            إذا (classp->user3p()) أرجع;  // will not get removed, no need to relink
            AstClass* ثابت parametrizedClassp = VN_CAST(classp->user4p(), Class);
            إذا (!parametrizedClassp) أرجع;
            Ast_نود_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_نود_FTask* ftaskp) {
                إذا (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    أرجع true;
                }
                أرجع خطأ;
            });
            إذا (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ثابت ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        لكل (AstClass* ثابت classp : m_state.m_paramClasses) {
            إذا (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                إذا (V3Error::error_کيراءن()) استمر;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(تولق_Deletep(classp->unlinkFrBack()), classp);
            } وإلا {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(خطأ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_نود::user3_Clear_ڤوکوق();
        لكل (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_نود* ثابت nodep) { nodep->user3(true); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_نود* ثابت nodep) {
            nodep->foreachLink([&](Ast_نود** ثابت linkpp, ثابت حرف* namep) {
                إذا (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_اتس() = افتراضي;
    VL_UNCOPYABLE(Param_اتس);
};

//######################################################################
// Param class functions

فراغ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    إذا (dump_ڤوکوق_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_اتس{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    إذا (dump_ڤوکوق_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ڤوکوق("param", 0, dump_ڤوکوق_Either_Level() >= 3);
}

فراغ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    لكل (AstVar* ثابت varp : rootp->deferredParamVarps()) {
        إذا (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
