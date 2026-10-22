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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Param.h"

#شامل "V3Case.h"
#شامل "V3Const.h"
#شامل "V3EmitV.h"
#شامل "V3HierBlock.h"
#شامل "V3LinkDotIfaceCapture.h"
#شامل "V3MemberMap.h"
#شامل "V3Os.h"
#شامل "V3Parse.h"
#شامل "V3Simulate.h"
#شامل "V3Stats.h"
#شامل "V3Unroll.h"
#شامل "V3Width.h"

#شامل <cctype>
#شامل <cstdlib>
#شامل <deque>
#شامل <map>
#شامل <memory>
#شامل <set>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ایستا AstClassRefDType* class_Ref_DType_Of_گره(Ast_گره* nodep) {
    وقتی (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_گره_DType* ثابت dtp = VN_CAST(nodep, گره_DType);
    بازگشت dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

رده ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ثابت V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ثابت std::string, Ast_گره_Module*>;
    using ParamConstMap = std::map<ثابت std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ثابت std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ثابت منطقی m_hierSubRun;  // Is in sub-run for hierarchical verilation
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

عمومی:
    ParameterizedHierBlocks(ثابت V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        برای (ثابت auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ثابت V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            برای (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    جدید File_سطر{File_سطر::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ثابت منطقی inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.درج({hierOpt.second.origName(), {}});
        }
        برای (Ast_گره_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گره_Module)) {
            اگر (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ثابت string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ثابت auto defParamIt = m_modParams.find(actualModName);
            اگر (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                        اگر (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ثابت AstBasicDType* ثابت dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        اگر (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    منطقی hierSubRun() ثابت { بازگشت m_hierSubRun; }
    منطقی isHierBlock(ثابت string& origName) ثابت {
        بازگشت m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_گره_Module* findByParams(ثابت string& origName, AstPin* firstPinp,
                                ثابت Ast_گره_Module* modp) {
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
        برای (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            منطقی found = درست;
            size_t paramIdx = 0;
            ثابت ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            برای (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->exprp()) ادامه;
                اگر (ثابت AstVar* ثابت modvarp = pinp->modVarp()) {
                    AstConst* ثابت constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ثابت auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ثابت منطقی untyped = m_untypedParams.شمار(paramIt->second);
                    AstConst* ثابت defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    اگر (defValuep && areSame(constp, defValuep)) {
                        اگر (!untyped || constp->same_مقدار_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ادامه;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ثابت auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    اگر (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = نادرست;
                        بشکن;
                    }
                    اگر (untyped && !constp->same_مقدار_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = نادرست;
                        بشکن;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            اگر (found && paramIdx == hierIt->second->params().size()) بشکن;
        }
        اگر (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_گره::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            بازگشت nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ثابت HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ثابت auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        اگر (it == m_hierBlockMod.end()) بازگشت nullptr;
        بازگشت it->second;
    }
    ایستا منطقی areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        اگر (pinValuep->isString()) {
            بازگشت pinValuep->رقم().toString() == hierOptParamp->رقم().toString();
        }

        اگر (hierOptParamp->isDouble()) {
            دوگانه var;
            اگر (pinValuep->isDouble()) {
                var = pinValuep->رقم().toDouble();
            } وگرنه {  // Cast from integer to real
                V3_عدد var_رقم{pinValuep, V3_عدد::Double{}, 0.0};
                var_رقم.opIToRD(pinValuep->رقم());
                var = var_رقم.toDouble();
            }
            بازگشت V3_عدد::epsilonEqual(var, hierOptParamp->رقم().toDouble());
        } وگرنه {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_عدد var_رقم{pinValuep, hierOptParamp->رقم().width()};
            اگر (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_رقم.opRToIRoundS(pinValuep->رقم());
            } وگرنه اگر (pinValuep->isSigned()) {
                var_رقم.opExtendS(pinValuep->رقم(), pinValuep->رقم().width());
            } وگرنه {
                var_رقم.opAssign(pinValuep->رقم());
            }
            V3_عدد isEq{pinValuep, 1};
            isEq.opEq(var_رقم, hierOptParamp->رقم());
            بازگشت isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

رده ParamProcessor final {
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
    using CloneMap = std::unordered_map<ثابت Ast_گره*, Ast_گره*>;
    ساختار ModInfo final {
        Ast_گره_Module* ثابت m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_گره_Module* modp)
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

    std::map<ثابت std::string, std::string> m_مقدار_Names;  // Parameter value text to its name
    std::set<std::string> m_مقدار_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ایستا constexpr std::string::size_type DIGEST_DIGITS = 8;

    ثابت Ast_گره_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_مقدار_Map = std::map<std::string, Ast_گره*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_گره_Module*, Default_مقدار_Map> m_default_Parameter_مقادیر;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ثابت Ast_گره_Module*, ثابت AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, صحیح>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, صحیح> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ثابت AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ایستا تهی makeSmallNames(Ast_گره_Module* modp) {
        std::vector<صحیح> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isGParam() || varp->isIfaceRef()) {
                    نویسه ch = varp->name()[0];
                    ch = std::toupper(ch);
                    اگر (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<صحیح>(ch)] * 256 + ch);
                    usedLetter[static_cast<صحیح>(ch)]++;
                }
            } وگرنه اگر (AstParamTypeDType* ثابت typep = VN_CAST(stmtp, ParamTypeDType)) {
                ثابت نویسه ch = 'T';
                typep->user3(usedLetter[static_cast<صحیح>(ch)] * 256 + ch);
                usedLetter[static_cast<صحیح>(ch)]++;
            }
        }
    }
    ایستا string paramSmallName(Ast_گره_Module* modp, Ast_گره* varp) {
        اگر (varp->user3() <= 1) makeSmallNames(modp);
        صحیح index = varp->user3() / 256;
        ثابت نویسه ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        وقتی (index) {
            st += cvtToStr(static_cast<نویسه>((index % 25) + 'A'));
            index /= 26;
        }
        بازگشت st;
    }

    ایستا منطقی is_Aggregate_Param_مقدار(ثابت Ast_گره* nodep) {
        بازگشت VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ایستا string param_مقدار_String(ثابت Ast_گره* nodep) {
        اگر (ثابت AstRefDType* ثابت refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string کلید = nodep->name();
        اگر (ثابت AstIfaceRefDType* ثابت ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            اگر (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                کلید = ifrtp->cellp()->modp()->name();
            } وگرنه اگر (ifrtp->ifacep()) {
                کلید = ifrtp->ifacep()->name();
            } وگرنه {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت dtypep
                   = VN_CAST(nodep, گره_UOr_Struct_DType)) {
            کلید += " ";
            کلید += dtypep->verilogKwd();
            کلید += " {";
            برای (ثابت Ast_گره* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                کلید += param_مقدار_String(memberp);
                کلید += ";";
            }
            کلید += "}";
        } وگرنه اگر (ثابت AstMemberDType* ثابت dtypep = VN_CAST(nodep, MemberDType)) {
            کلید += " ";
            کلید += param_مقدار_String(dtypep->subDTypep());
        } وگرنه اگر (ثابت AstBasicDType* ثابت dtypep = VN_CAST(nodep, BasicDType)) {
            اگر (dtypep->isSigned()) کلید += " signed";
            اگر (dtypep->isRanged()) {
                کلید += "[" + cvtToStr(dtypep->چپ()) + ":" + cvtToStr(dtypep->راست()) + "]";
            }
        } وگرنه اگر (ثابت AstPackArrayDType* ثابت dtypep = VN_CAST(nodep, PackArrayDType)) {
            کلید += "[";
            کلید += cvtToStr(dtypep->چپ());
            کلید += ":";
            کلید += cvtToStr(dtypep->راست());
            کلید += "] ";
            کلید += param_مقدار_String(dtypep->subDTypep());
        } وگرنه اگر (ثابت AstPattern* ثابت patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            کلید += "'{";
            برای (ثابت AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                اگر (memp->isDefault()) کلید += "default";
                اگر (ثابت AstText* ثابت textp = VN_CAST(memp->keyp(), Text)) {
                    کلید += textp->text();
                } وگرنه اگر (memp->keyp()) {
                    کلید += param_مقدار_String(memp->keyp());
                }
                کلید += ":";
                اگر (memp->repp()) کلید += param_مقدار_String(memp->repp()) + "x";
                برای (ثابت Ast_گره_عبارت* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), گره_عبارت)) {
                    کلید += param_مقدار_String(valuep) + ";";
                }
                کلید += ",";
            }
            کلید += "}";
        } وگرنه اگر (ثابت AstConsArray* ثابت consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            کلید += "{";
            برای (ثابت AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                کلید += cvtToStr(itemp->index()) + ":" + param_مقدار_String(itemp->valuep()) + ",";
            }
            اگر (consArrayp->defaultp()) {
                کلید += "default:" + param_مقدار_String(consArrayp->defaultp()) + ",";
            }
            کلید += "}";
        } وگرنه اگر (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep, ConsStruct)) {
            کلید += "{";
            برای (ثابت AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                کلید += param_مقدار_String(itemp->valuep());
                کلید += ",";
            }
            کلید += "}";
        } وگرنه اگر (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            اگر (classRefp->classp()) {
                ثابت string& className = classRefp->classp()->name();
                ثابت string& origName = classRefp->classp()->origName();
                ثابت منطقی isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                اگر (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } وگرنه اگر (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    کلید = className;
                } وگرنه {
                    // Unspecialized class with no params - use origName
                    کلید = origName;
                }
            } وگرنه {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            کلید = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } وگرنه اگر (ثابت AstQueueDType* ثابت dtypep = VN_CAST(nodep, QueueDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[$";
            اگر (dtypep->boundConst()) کلید += ":" + cvtToStr(dtypep->boundConst());
            کلید += "]";
        } وگرنه اگر (ثابت AstDynArrayDType* ثابت dtypep = VN_CAST(nodep, DynArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[]";
        } وگرنه اگر (ثابت AstAssocArrayDType* ثابت dtypep = VN_CAST(nodep, AssocArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->کلید_DTypep()) + "]";
        } وگرنه اگر (ثابت AstWildcardArrayDType* ثابت dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[*]";
        } وگرنه اگر (ثابت AstEnumDType* ثابت dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            کلید += " enum {";
            برای (ثابت AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const);
                کلید += itemp->name() + "=" + (constp ? constp->رقم().ascii() : "?") + ";";
            }
            کلید += "}";
        } وگرنه اگر (ثابت Ast_گره_DType* ثابت dtypep = VN_CAST(nodep, گره_DType)) {
            کلید += dtypep->prettyDTypeName(درست);
        }
        UASSERT_OBJ(!کلید.empty(), nodep, "Parameter yielded no value string");
        بازگشت کلید;
    }
    // As paramValueString(), for the element or key type of a container
    ایستا string paramElemString(ثابت Ast_گره_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ثابت AstClassRefDType* ثابت classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        اگر (classRefp && classRefp->paramsp()) بازگشت classRefp->prettyDTypeName(درست);
        بازگشت param_مقدار_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ایستا string digestSuffix(ثابت string& text, std::set<string>& usedr) {
        ثابت string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        وقتی (digits < hex.size() && !usedr.درج(hex.substr(0, digits)).second) ++digits;
        بازگشت hex.substr(0, digits);
    }
    string param_مقدار_عدد(Ast_گره* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        اگر (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ثابت string text = param_مقدار_String(nodep);
        ثابت auto pair = m_مقدار_Names.emplace(text, "");
        اگر (pair.second) pair.first->second = "z" + digestSuffix(text, m_مقدار_Suffixes);
        بازگشت pair.first->second;
    }
    string moduleCalcName(ثابت Ast_گره_Module* srcModp, ثابت string& longname) {
        string newname = longname;
        اگر (longname.length() > 30) {
            ثابت auto pair = m_longMap.emplace(longname, "");
            اگر (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        بازگشت newname;
    }
    ایستا Ast_گره_DType* arraySubDTypep(Ast_گره_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            بازگشت adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        اگر (ثابت AstBracketArrayDType* ثابت adtypep = VN_CAST(nodep, BracketArrayDType)) {
            بازگشت adtypep->subDTypep();
        }
        بازگشت nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ایستا Ast_گره_DType* arraySubDTypeDeepp(Ast_گره_DType* nodep) {
        وقتی (Ast_گره_DType* ثابت subp = arraySubDTypep(nodep)) nodep = subp;
        بازگشت nodep;
    }
    ایستا منطقی isString(Ast_گره_DType* nodep) {
        اگر (AstBasicDType* ثابت basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            بازگشت basicp->isString();
        بازگشت نادرست;
    }
    تهی collectPins(CloneMap* clonemapp, Ast_گره_Module* modp, منطقی originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت Ast_گره* originalParamp = nullptr;
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } وگرنه اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            اگر (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            اگر (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    تهی relinkPins(ثابت CloneMap* clonemapp, AstPin* startpinp) {
        برای (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ثابت auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } وگرنه اگر (pinp->modPTypep()) {
                ثابت auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } وگرنه {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    تهی relinkPinsByName(AstPin* startpinp, Ast_گره_Module* modp) {
        std::map<ثابت string, AstVar*> nameToPin;
        برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        برای (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                ثابت auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    ایستا منطقی hasDescendantDefparams(ثابت Ast_گره_Module* modp,
                                       std::set<ثابت Ast_گره_Module*>& visited) {
        اگر (!visited.درج(modp).second) بازگشت نادرست;
        برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت cellp = VN_CAST(stmtp, Cell);
            اگر (!cellp) ادامه;
            برای (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    بازگشت درست;
                }
            }
            اگر (Ast_گره_Module* ثابت childModp = cellp->modp()) {
                اگر (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    بازگشت درست;
                }
            }
        }
        visited.erase(modp);
        بازگشت نادرست;
    }
    ایستا منطقی hasDescendantDefparams(ثابت Ast_گره_Module* modp) {
        std::set<ثابت Ast_گره_Module*> visited;
        بازگشت hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ثابت AstPin* nestedDefparamp(ثابت Ast_گره_Module* modp) {
        ثابت auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        اگر (!pair.second) بازگشت pair.first->second;  // Searched, or being searched
        ثابت AstPin* foundp = nullptr;
        modp->exists([&](ثابت AstCell* cellp) {
            برای (ثابت AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->paramPath().empty()) foundp = pinp;
            }
            اگر (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            بازگشت foundp != nullptr;
        });
        pair.first->second = foundp;
        بازگشت foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    تهی checkSupportedParam(Ast_گره_Module* modp, AstPin* pinp) ثابت {
        // ConsArray is not supported because that can not be set via -G
        // option.
        اگر (pinp->modVarp()) {
            منطقی supported = نادرست;
            اگر (ثابت AstConst* ثابت constp = VN_CAST(pinp->exprp(), Const)) {
                supported = درست;
                اگر (constp->رقم().isString()
                    && !V3HierBlock::stringParamPassable(constp->رقم().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            اگر (!supported) {
                pinp->v3error(
                    Ast_گره::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } وگرنه اگر (ثابت AstParamTypeDType* ثابت typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_گره::prettyNameQ(modp->origName()));
        }
    }
    منطقی moduleExists(ثابت string& modName) ثابت {
        اگر (m_allModuleNames.find(modName) != m_allModuleNames.end()) بازگشت درست;
        اگر (m_modNameMap.find(modName) != m_modNameMap.end()) بازگشت درست;
        بازگشت نادرست;
    }

    string parameterizedHierBlockName(Ast_گره_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_گره*> pins;

        AstPin* pinp = paramPinsp;
        وقتی (pinp) {
            checkSupportedParam(modp, pinp);
            اگر (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                اگر (!pinp->exprp()) ادامه;
                اگر (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } وگرنه اگر (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), گره_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ثابت auto pair = m_default_Parameter_مقادیر.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        اگر (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            اگر (ثابت AstPin* ثابت defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_گره::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_مقدار_Map params;
            برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    اگر (varp->isGParam()) {
                        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } وگرنه اگر (AstParamTypeDType* ثابت p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_گره* ثابت dtypep = static_cast<Ast_گره*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ثابت auto paramsIt = pair.first;
        اگر (paramsIt->second.empty()) بازگشت modp->origName();  // modp has no parameter

        string longname = modp->origName();
        برای (auto&& default_مقدار : paramsIt->second) {
            ثابت auto pinIt = pins.find(default_مقدار.first);
            // If the pin does not have a value assigned, use the default one.
            ثابت Ast_گره* ثابت nodep = pinIt == pins.end() ? default_مقدار.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_مقدار.first + "=";
            // constp can be nullptr

            اگر (ثابت AstConst* ثابت p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->رقم().ascii(نادرست);
            } وگرنه اگر (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_درخت(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ثابت auto iter = m_longMap.find(longname);
        اگر (iter != m_longMap.end()) بازگشت iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.درج(longname);
        وقتی (درست) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ثابت string hashStr = hashStrGen.digestHex();
            برای (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                اگر (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                اگر (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    بازگشت newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.درج(V3Os::trueRandom(64));
        }
    }
    تهی replaceRefsRecurse(Ast_گره* ثابت nodep, ثابت AstClass* ثابت oldClassp,
                            AstClass* ثابت newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        اگر (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            اگر (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } وگرنه اگر (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            اگر (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } وگرنه اگر (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            اگر (typedefp->subDTypep()) {
                اگر (AstClassRefDType* ثابت classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    اگر (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } وگرنه اگر (Ast_گره_FTask_Ref* ثابت ftaskRefp = VN_CAST(nodep, گره_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            اگر (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            اگر (Ast_گره_FTask* ثابت oldTaskp = ftaskRefp->taskp()) {
                اگر (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        اگر (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        اگر (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        اگر (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        اگر (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        اگر (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    رده VarXRefRelinkVisitor final : عمومی VNVisitor {
        Ast_گره_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_گره_Module*> m_varModuleMap;  // Cache var->module lookups

    عمومی:
        explicit VarXRefRelinkVisitor(Ast_گره_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    خصوصی:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_گره_Module* findVarModule(AstVar* varp) {
            ثابت auto it = m_varModuleMap.find(varp);
            اگر (it != m_varModuleMap.end()) بازگشت it->second;
            Ast_گره_Module* varModp = nullptr;
            برای (Ast_گره* np = varp; np; np = np->backp()) {
                اگر (Ast_گره_Module* ثابت modp = VN_CAST(np, گره_Module)) {
                    varModp = modp;
                    بشکن;
                }
            }
            m_varModuleMap[varp] = varModp;
            بازگشت varModp;
        }

        تهی visit(AstVarXRef* nodep) override {
            AstVar* ثابت varp = nodep->varp();
            اگر (!varp) {
                iterateChildren(nodep);
                بازگشت;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ثابت string& dotted = nodep->dotted();
            اگر (dotted.empty()) {
                iterateChildren(nodep);
                بازگشت;
            }

            ثابت size_t dotPos = dotted.find('.');
            ثابت string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            اگر (portName.empty()) {
                iterateChildren(nodep);
                بازگشت;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            برای (Ast_گره* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varChkp = VN_CAST(stmtp, Var)) {
                    اگر (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        بشکن;
                    }
                }
            }
            اگر (!portVarp) {
                iterateChildren(nodep);
                بازگشت;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ثابت irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            اگر (!irefp) {
                iterateChildren(nodep);
                بازگشت;
            }

            Ast_گره_Module* ثابت newIfacep = irefp->ifaceViaCellp();
            اگر (!newIfacep) {
                iterateChildren(nodep);
                بازگشت;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_گره_Module* ثابت varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            اگر (varModp && varModp != newIfacep) {
                برای (Ast_گره* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت newVarp = VN_CAST(stmtp, Var)) {
                        اگر (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            بشکن;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    تهی fixupCrossModuleRefDTypes(Ast_گره_Module* newModp) {
        اگر (!V3LinkDotIfaceCapture::enabled()) بازگشت;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        صحیح fixed_شمار = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ثابت VIfaceCaptureTag* ثابت tagp = V3LinkDotIfaceCapture::captureTag(refp);
            اگر (!tagp) بازگشت;

            Ast_گره_Module* ثابت correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            اگر (!correctModp || correctModp->dead()) بازگشت;
            اگر (correctModp->parameterizedTemplate()) بازگشت;

            منطقی fixed = نادرست;
            اگر (refp->typedefp()) {
                اگر (AstTypedef* ثابت newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    اگر (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = درست;
                }
            }
            اگر (!fixed && refp->refDTypep()) {
                اگر (Ast_گره_DType* ثابت newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = درست;
                }
            }
            اگر (fixed) ++fixed_شمار;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_شمار);
    }

    // Return true on success, false on error
    منطقی deepCloneModule(Ast_گره_Module* srcModp, Ast_گره* ifErrorp, AstPin* paramsp,
                         ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_گره_Module* newModp;
        اگر (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_درخت(نادرست), گره_Module);
        } وگرنه {
            newModp = srcModp->clone_درخت(نادرست);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        اگر (AstIface* ثابت newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(درست);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(نادرست);

        newModp->name(newname);
        newModp->user2(نادرست);  // We need to re-recurse this module once changed
        newModp->recursive(نادرست);
        newModp->recursiveClone(نادرست);
        newModp->hasGParam(نادرست);
        // Recursion may need level cleanups
        اگر (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        اگر ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_درخت(), newModp);
            بازگشت نادرست;
        }

        اگر (AstClass* ثابت newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_گره* insertp = srcModp;
        وقتی (VN_IS(insertp->nextp(), گره_Module)
               && VN_AS(insertp->nextp(), گره_Module)->level() <= newModp->level()) {
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
        منطقی needRelinkByName = نادرست;
        اگر (paramsp) {
            برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = درست;
                    بشکن;
                }
            }
        }
        اگر (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } وگرنه {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        برای (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
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
        اگر (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->exprp()) {
                اگر (AstVar* ثابت modvarp = pinp->modVarp()) {
                    Ast_گره* ثابت newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ثابت exprp = VN_CAST(newp, Const);
                    AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                    ثابت منطقی overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    اگر (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_درخت();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_درخت(نادرست));
                    modvarp->overriddenParam(overridden);
                } وگرنه اگر (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    Ast_گره_DType* ثابت dtypep = VN_AS(pinp->exprp(), گره_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    اگر (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_درخت();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_درخت(نادرست));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        بازگشت درست;
    }
    ثابت ModInfo* moduleFindOrClone(Ast_گره_Module* srcModp, Ast_گره* ifErrorp, AstPin* paramsp,
                                     ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        اگر (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } وگرنه {
            اگر (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                بازگشت nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ثابت ModInfo* ثابت modInfop = &(it->second);
        بازگشت modInfop;
    }

    تهی convertToStringp(Ast_گره* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        اگر (constp && !constp->رقم().isString()) {
            constp->replaceWith(
                جدید AstConst{constp->fileline(), AstConst::String{}, constp->رقم().toString()});
            VL_DO_DANGLING(constp->delete_درخت(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    تهی constify_Member_مقدار(AstVar* varp, std::set<AstVar*>& inProgress) {
        اگر (!varp->valuep() || VN_IS(varp->valuep(), Const)) بازگشت;
        اگر (!inProgress.درج(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_درخت();
            varp->valuep(جدید AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            بازگشت;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ثابت AstVarRef* refp) {
            AstVar* ثابت refVarp = refp->varp();
            اگر (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.درج(refVarp);
            }
        });
        برای (AstVar* ثابت sibp : siblings) constify_Member_مقدار(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ایستا AstParseRef* memberParseRef(Ast_گره* nodep) {
        اگر (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) بازگشت refp;
        Ast_گره_Pre_Sel* ثابت selp = VN_CAST(nodep, گره_Pre_Sel);
        بازگشت selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ایستا تهی replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_گره_عبارت* valuep) {
        اگر (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } وگرنه {
            Ast_گره_Pre_Sel* ثابت selp = VN_AS(dotp->rhsp(), گره_Pre_Sel);
            اگر (AstAttrOf* ثابت attrp = selp->attrp()) {
                اگر (Ast_گره* ثابت oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_درخت(نادرست));
                    VL_DO_DANGLING(oldFromp->delete_درخت(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_درخت(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_درخت(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ایستا منطقی isDeferredMemberBase(ثابت Ast_گره_عبارت* nodep) {
        اگر (VN_IS(nodep, MemberSel)) بازگشت درست;
        اگر (ثابت Ast_گره_Pre_Sel* ثابت selp = VN_CAST(nodep, گره_Pre_Sel)) {
            ثابت Ast_گره_DType* ثابت fromDTypep = selp->fromp()->dtypep();
            ثابت Ast_گره_Array_DType* ثابت arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), گره_Array_DType) : nullptr;
            اگر (arrayDTypep) {
                ثابت Ast_گره_DType* ثابت elemDTypep = arrayDTypep->subDTypep();
                بازگشت elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), گره_UOr_Struct_DType);
            }
            بازگشت isDeferredMemberBase(selp->fromp());
        }
        ثابت Ast_گره_DType* ثابت dtypep = nodep->dtypep();
        بازگشت dtypep && VN_IS(dtypep->skipRefOrNullp(), گره_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ایستا تهی resolveMemberDot(AstDot* dotp) {
        Ast_گره_عبارت* ثابت lhsp = VN_CAST(dotp->lhsp(), گره_عبارت);
        AstParseRef* ثابت memberRefp = memberParseRef(dotp->rhsp());
        اگر (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) بازگشت;
        AstMemberSel* ثابت newp = جدید AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    تهی resolveDotToTypedef(Ast_گره* exprp) {
        AstDot* ثابت dotp = VN_CAST(exprp, Dot);
        اگر (!dotp) بازگشت;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ثابت classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        اگر (!classRefp) {
            resolveMemberDot(dotp);
            بازگشت;
        }
        AstParseRef* ثابت parseRefp = memberParseRef(dotp->rhsp());
        اگر (!parseRefp) بازگشت;

        ثابت AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ثابت paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        اگر (paramTypep) {
            Ast_گره_DType* ثابت dtypep = paramTypep->subDTypep();
            AstClassRefDType* ثابت classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            اگر (classRefDTypep) {
                AstClass* ثابت srcClassp = classRefDTypep->classp();
                اگر (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    اگر (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        اگر (classRefp->paramsp()) {
            AstClass* ثابت srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            اگر (srcClassp && srcClassp->hasGParam()) {
                اگر (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        اگر (AstClassRefDType* ثابت crp = class_Ref_DType_Of_گره(classRefp->classOrPackageNodep())) {
            اگر (AstClass* ثابت srcClassp = crp->classp()) {
                اگر (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    اگر (AstClass* ثابت newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        اگر (!lhsClassp) بازگشت;

        Ast_گره* ثابت memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        اگر (AstTypedef* ثابت tdefp = VN_CAST(memberp, Typedef)) {
            اگر (parseRefp != dotp->rhsp()) بازگشت;
            AstRefDType* ثابت refp = جدید AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_درخت(), dotp);
        } وگرنه اگر (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    تهی substituteParamMember(AstDot* ثابت dotp, AstParseRef* ثابت memberRefp,
                               AstVar* ثابت varp) {
        اگر (!varp->isParam() || !varp->valuep()) بازگشت;
        اگر (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_مقدار(varp, inProgress);
        }
        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
        اگر (!constp) بازگشت;
        AstConst* ثابت newp = constp->clone_درخت(نادرست);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    تهی resolveParamClassRefDType(AstRefDType* refp) {
        اگر (refp->typedefp() || refp->refDTypep()) بازگشت;

        AstClassOrPackageRef* ثابت classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        اگر (!classRefp) بازگشت;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        اگر (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            بازگشت;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_گره* targetp = classRefp->classOrPackageNodep();
        وقتی (ثابت AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        اگر (Ast_گره_DType* ثابت dtargetp = VN_CAST(targetp, گره_DType))
            targetp = dtargetp->skipRefOrNullp();
        اگر (!targetp) بازگشت;
        AstClassRefDType* ثابت classRefDTypep = VN_CAST(targetp, ClassRefDType);
        اگر (!classRefDTypep) بازگشت;
        srcClassp = classRefDTypep->classp();
        اگر (!srcClassp) بازگشت;

        AstClass* newClassp = srcClassp;
        اگر (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ثابت typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        اگر (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ایستا منطقی defaultParamsHaveTypeQueryOnParamType(ثابت AstClassRefDType* nodep) {
        منطقی found = نادرست;
        nodep->foreach([&](ثابت AstAttrOf* attrp) {
            اگر (found || !attrp->attrType().isTypeQuery()) بازگشت;
            ثابت AstRefDType* ثابت refp = VN_CAST(attrp->fromp(), RefDType);
            اگر (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = درست;
        });
        بازگشت found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    منطقی classTypeMatchesDefaultClone(ثابت Ast_گره_DType* exprp, ثابت Ast_گره_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ثابت auto* ثابت عبارت_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ثابت auto* ثابت origClassRefp = VN_CAST(origp, ClassRefDType);
        اگر (!عبارت_Class_Refp || !origClassRefp) بازگشت نادرست;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ثابت Ast_گره_Module* ثابت defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        اگر (defaultClonep && defaultClonep == عبارت_Class_Refp->classp()) بازگشت درست;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        اگر (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) بازگشت نادرست;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        اگر (!origClassRefp->classp()->hasGParam()) بازگشت نادرست;
        اگر (origClassRefp->classp()->origName() != عبارت_Class_Refp->classp()->origName())
            بازگشت نادرست;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        اگر (!m_defaultCloneInProgress.درج(origClassRefp->classp()).second) بازگشت نادرست;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ثابت origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_درخت(نادرست));
        Ast_گره_Module* ثابت resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ثابت منطقی match = resolvedModp && VN_CAST(resolvedModp, Class) == عبارت_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_درخت(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        بازگشت match;
    }

    ایستا منطقی paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        اگر (exprp->رقم().width() == origp->رقم().width()) بازگشت نادرست;
        اگر (exprp->رقم().isOpaque()) بازگشت نادرست;
        اگر (origp->رقم().isOpaque()) بازگشت نادرست;
        ثابت صحیح maxWidth = std::max(exprp->رقم().width(), origp->رقم().width());
        V3_عدد عبارت_رقم{exprp, maxWidth};
        اگر (exprp->رقم().isSigned()) {
            عبارت_رقم.opExtendS(exprp->رقم(), exprp->رقم().width());
        } وگرنه {
            عبارت_رقم.opAssign(exprp->رقم());
        }
        V3_عدد orig_رقم{origp, maxWidth};
        اگر (origp->رقم().isSigned()) {
            orig_رقم.opExtendS(origp->رقم(), origp->رقم().width());
        } وگرنه {
            orig_رقم.opAssign(origp->رقم());
        }
        V3_عدد isEq{exprp, 1};
        isEq.opEq(عبارت_رقم, orig_رقم);
        بازگشت isEq.isNeqZero();
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
    تهی resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (!pinp->exprp()) ادامه;
            اگر (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ادامه;
            ثابت Ast_گره_Var_Ref* ثابت fwdRefp = VN_CAST(pinp->exprp(), گره_Var_Ref);
            اگر (!fwdRefp) ادامه;
            ثابت AstIfaceRefDType* ثابت resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ثابت newIrefp = جدید AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            اگر (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_درخت(درست));
            }
            pinp->exprp()->unlinkFrBack()->delete_درخت();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    تهی name_By_Pin_مقدار(AstPin* pinp, Ast_گره_Module* srcModp, AstVar* modvarp, string& longnamer,
                        منطقی& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_مقدار_عدد(pinp->exprp());
        any_overridesr = درست;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    تهی cellPinCleanup(Ast_گره* nodep, AstPin* pinp, AstPin* paramsp, Ast_گره_Module* srcModp,
                        string& longnamer, منطقی& any_overridesr,
                        std::unordered_set<ثابت AstPin*>& overridingTypePinsr) {
        اگر (!pinp->exprp()) بازگشت;  // No-connect
        اگر (AstVar* ثابت modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            اگر (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وگرنه اگر (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_مقدار(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } وگرنه {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                اگر (is_Aggregate_Param_مقدار(pinp->exprp())) {
                    name_By_Pin_مقدار(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    بازگشت;
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
                اگر (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    ثابت منطقی dependent = modvarp->valuep()->exists(
                        [](Ast_گره* np) { بازگشت VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    اگر (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                اگر (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ثابت exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                اگر (exprp && !exprp->رقم().isDouble() && !exprp->رقم().isString()) {
                    AstVar* cloneVarp = modvarp->clone_درخت(نادرست);
                    منطقی cloneVarpUnresolved = نادرست;
                    اگر (Ast_گره* ثابت oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_درخت(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_درخت(نادرست));
                    اگر (Ast_گره_DType* ثابت origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        اگر (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_درخت();
                        cloneVarp->childDTypep(origDTypep->clone_درخت(نادرست));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr صحیح maxSubstIters = 1000;
                        برای (صحیح it = 0; it < maxSubstIters; ++it) {
                            منطقی any = نادرست;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ثابت targetp = varrefp->varp();
                                Ast_گره* replacep = nullptr;
                                برای (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    اگر (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        اگر (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_درخت(نادرست);
                                        }
                                        بشکن;
                                    }
                                }
                                اگر (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_درخت(نادرست);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ثابت patp = VN_CAST(replacep, Pattern);
                                اگر (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_درخت(نادرست));
                                }
                                اگر (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_درخت(), varrefp);
                                    any = درست;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_گره_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ثابت ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                اگر (!ptdp) بازگشت;
                                AstPin* overridePinp = nullptr;
                                برای (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    اگر (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        بشکن;
                                    }
                                }
                                Ast_گره_DType* ثابت substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), گره_DType)
                                                   : ptdp->subDTypep();
                                اگر (substp) toReplace.emplace_back(refp, substp);
                            });
                            برای (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ثابت refp = it->first;
                                refp->replaceWith(it->second->clone_درخت(نادرست));
                                VL_DO_DANGLING(refp->delete_درخت(), refp);
                                any = درست;
                            }
                            اگر (!any) بشکن;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_گره* np) {
                            اگر (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = درست;
                                بازگشت;
                            }
                            اگر (AstEnumItemRef* ثابت itemRefp = VN_CAST(np, EnumItemRef)) {
                                اگر (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = درست;
                                }
                                بازگشت;
                            }
                            AstRefDType* ثابت refp = VN_CAST(np, RefDType);
                            اگر (!refp) بازگشت;
                            اگر (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = درست;
                            }
                            // A type declared in the template must not be widthed there
                            اگر (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = درست;
                            }
                        });
                    }
                    اگر (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        اگر (AstConst* ثابت widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            اگر (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_درخت(), cloneVarp);
                }
                AstConst* ثابت namingExprp = normedNamep ? normedNamep : exprp;
                اگر (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_گره* ثابت pinExprp = pinp->exprp();
                    pinExprp->replaceWith(جدید AstConst{pinp->fileline(), AstConst::Widthed_مقدار{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_درخت(), pinExprp);
                } وگرنه اگر (origp
                           && (exprp->same_درخت(origp)
                               || (exprp->رقم().width() == origp->رقم().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } وگرنه اگر (namingExprp->رقم().isDouble() || namingExprp->رقم().isString()
                           || namingExprp->رقم().isFourState()
                           || namingExprp->رقم().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_مقدار_عدد(namingExprp));
                    any_overridesr = درست;
                } وگرنه {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->رقم().ascii(نادرست));
                    any_overridesr = درست;
                }
                اگر (normedNamep) VL_DO_DANGLING(normedNamep->delete_درخت(), normedNamep);
            }
        } وگرنه اگر (AstParamTypeDType* ثابت modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_گره_DType* rawTypep = VN_CAST(pinp->exprp(), گره_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            منطقی skipWidthForTemplateStruct = نادرست;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_گره_DType* ثابت resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                اگر (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_گره_Module* ثابت ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    اگر (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = درست;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                اگر (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_گره_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ثابت Ast_گره_DType* origp = modvarp->skipRefToNonRefp();
            اگر (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وگرنه اگر (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } وگرنه {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                اگر (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), گره_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                اگر (!modvarp->fwdType().is_گره_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                اگر (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } وگرنه {
                    اگر (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), گره_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    اگر (AstClassRefDType* ثابت classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        اگر (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } وگرنه {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    تهی cellInterfaceCleanup(AstPin* pinsp, Ast_گره_Module* srcModp, string& longnamer,
                              منطقی& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        برای (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ثابت AstVar* ثابت modvarp = pinp->modVarp();
            اگر (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ادامه;
            }
            اگر (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ثابت portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ثابت Ast_گره* ثابت exprp = pinp->exprp();
                اگر (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ادامه;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                اگر (ثابت Ast_گره_Var_Ref* ثابت vrp = VN_CAST(exprp, گره_Var_Ref)) {
                    اگر (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                اگر (!pinIrefp && exprp) {
                    ثابت Ast_گره* basep = exprp;
                    وقتی (ثابت Ast_گره_Pre_Sel* ثابت selp = VN_CAST(basep, گره_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    اگر (ثابت AstVarRef* ثابت vrp = VN_CAST(basep, VarRef)) {
                        اگر (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                اگر (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } وگرنه اگر (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } وگرنه {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    اگر (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_مقدار_عدد(pinIrefp));
                        any_overridesr = درست;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        اگر (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_گره::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_گره::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    تهی storeOriginalParams(Ast_گره_Module* ثابت classp) {
        برای (Ast_گره* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_گره* originalParamp = nullptr;
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isGParam()) originalParamp = varp->clonep();
            } وگرنه اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            اگر (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    تهی genericInterfaceVarSetup(ثابت AstPin* ثابت paramsp, ثابت AstPin* ثابت pinsp) {
        std::unordered_map<string, ثابت AstPin*> paramspMap;
        برای (ثابت AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.درج({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        اگر (paramspMap.empty()) بازگشت;

        برای (ثابت Ast_گره* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                اگر (AstVar* ثابت varp = pinp->modVarp()) {
                    Ast_گره_DType* dtypep = varp->childDTypep()->elemDTypep();
                    اگر (AstIfaceGenericDType* ثابت ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ثابت auto iter = paramspMap.find(varp->name());
                        اگر (iter == paramspMap.end()) ادامه;
                        Ast_گره* ثابت backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ثابت AstPin* ثابت paramp = iter->second;
                        paramspMap.erase(iter);
                        ثابت AstIfaceRefDType* ثابت ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ثابت newIfacerefp = جدید AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        اگر (auto* ثابت arrDtp = VN_CAST(backp, گره_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } وگرنه اگر (auto* ثابت arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } وگرنه {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.افزودن_Deletep(ifaceGDTypep), ifaceGDTypep);
                        اگر (paramspMap.empty()) بازگشت;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    تهی resolveDefaultParams(Ast_گره* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        اگر (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            اگر (!classp) بازگشت;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } وگرنه اگر (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } وگرنه {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        برای (Ast_گره* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                اگر (!paramTypep->isGParam()) ادامه;
                m_paramIndex.emplace(paramTypep, static_cast<صحیح>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_گره_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        برای (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* ثابت paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ثابت reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            اگر (!reqDtp) ادامه;

            // If default is a reference to another param type, record dependency
            اگر (AstRefDType* ثابت refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                اگر (AstParamTypeDType* ثابت sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    اگر (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    ادامه;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            اگر (Ast_گره_DType* ثابت dtp = VN_CAST(reqDtp->lhsp(), گره_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ثابت typep = pinp->modPTypep();
            ثابت auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            اگر (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } وگرنه {
                pinsByIndex.افزودن_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        برای (منطقی resolvedSome = درست; resolvedSome;) {
            resolvedSome = نادرست;
            برای (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                اگر (pinsByIndex[paramIdx]) ادامه;
                ثابت صحیح sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                اگر (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_درخت(نادرست);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                اگر (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_گره_DType* ثابت dtypep = defaultTypeNodes[paramIdx];
                    newPinp = جدید AstPin{dtypep->fileline(), static_cast<صحیح>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_درخت(نادرست)};
                }

                اگر (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(درست);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    اگر (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } وگرنه اگر (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    اگر (!paramsp) paramsp = newPinp;
                    resolvedSome = درست;
                }
            }
        }
    }

    Ast_گره_Module* گره_Deparam_Common(Ast_گره* nodep, Ast_گره_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, منطقی any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        اگر (debug() >= 9 && paramsp) paramsp->dump_درخت_And_Next(cout, "-  cellparams: ");

        اگر (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } وگرنه {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ثابت AstPin*> overridingTypePins;
            برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->exprp()) ادامه;  // An empty override keeps the default
                اگر (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ادامه;
                }
                AstVar* ثابت modvarp = pinp->modVarp();
                اگر (!modvarp || !modvarp->isGParam()) ادامه;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ثابت AstPattern* ثابت patternp = VN_CAST(pinp->exprp(), Pattern);
                اگر (patternp && !patternp->childDTypep()) ادامه;
                اگر (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_مقدار(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    اگر (!overridingTypePins.شمار(pinp)) ادامه;
                    Ast_گره_DType* ثابت typep = VN_AS(pinp->exprp(), گره_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_مقدار_عدد(typep->skipRefToNonRefp());
                    any_overrides = درست;
                    ادامه;
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
        منطقی defaultsResolved = نادرست;
        اگر (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            برای (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (pinp->modPTypep()) {
                    any_overrides = درست;
                    defaultsResolved = درست;
                    بشکن;
                }
            }
        }
        اگر (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = درست;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_گره_Module* newModp = nullptr;
        اگر (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_گره_Module* ثابت paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            اگر (!paramedModp) بازگشت nullptr;  // Unsupported, already reported
            paramedModp->dead(نادرست);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } وگرنه اگر (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            اگر (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_گره_Module* گره_Copyp = srcModp->clone_درخت(نادرست);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << گره_Copyp);
                m_deleter.افزودن_Deletep(گره_Copyp);  // nodeCopyp used past here
                srcModp->user3p(گره_Copyp);
                storeOriginalParams(گره_Copyp);
            }
            newModp = srcModp;
        } وگرنه {
            ثابت string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ثابت ModInfo* ثابت modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            اگر (!modInfop) بازگشت nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ثابت منطقی cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ثابت newClassp = VN_CAST(newModp, Class);
        اگر (newClassp && !newClassp->user2()) m_specializedClassps.افزودن_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        اگر (defaultsResolved) srcModp->user4p(newModp);

        برای (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        اگر (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_درخت(), paramsp);
        بازگشت newModp;
    }

    Ast_گره_Module* cellDeparam(AstCell* nodep, Ast_گره_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_گره_Module* ثابت newModp = گره_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        اگر (!newModp) بازگشت nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(نادرست);
        بازگشت newModp;
    }
    Ast_گره_Module* ifaceRefDeparam(AstIfaceRefDType* ثابت nodep, Ast_گره_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        اگر (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ثابت string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ثابت string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ثابت size_t ownerPos = ownerBaseName.find("__");
            اگر (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ثابت size_t srcPos = srcBaseName.find("__");
            اگر (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            اگر (ownerBaseName == srcBaseName) {
                منطقی allOwnParams = درست;
                برای (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    اگر (AstRefDType* ثابت refp = VN_CAST(pinp->exprp(), RefDType)) {
                        اگر (AstParamTypeDType* ثابت ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_گره_Module* ثابت ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            اگر (ptdOwnerp != m_modp) allOwnParams = نادرست;
                        } وگرنه {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } وگرنه {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                اگر (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    اگر (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_درخت();
                    بازگشت ownerIfacep;
                }
            }
        }

        Ast_گره_Module* ثابت newModp
            = گره_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, نادرست);
        اگر (!newModp) بازگشت nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        بازگشت newModp;
    }
    Ast_گره_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_گره_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_گره_Module* ثابت newModp
            = گره_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, نادرست);
        اگر (!newModp) بازگشت nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ثابت refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ثابت newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        اگر (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            اگر (AstTypedef* ثابت typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        بازگشت newModp;
    }
    Ast_گره_Module* classRefDeparam(AstClassRefDType* nodep, Ast_گره_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_گره_Module* ثابت newModp
            = گره_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, نادرست);
        اگر (!newModp) بازگشت nullptr;
        AstClass* ثابت newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        بازگشت newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    رده DeferredResolverVisitor final : عمومی VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ثابت Ast_گره*> m_reachedDecls;

        منطقی firstReach(ثابت Ast_گره* ثابت declp) { بازگشت m_reachedDecls.درج(declp).second; }

        تهی iterateParamType(AstParamTypeDType* ثابت paramTypep) {
            اگر (firstReach(paramTypep)) iterate(paramTypep);
        }
        تهی iterateTypedef(AstTypedef* ثابت tdefp) {
            اگر (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
        تهی visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_گره* ثابت targetp = nodep->classOrPackageNodep();
            اگر (AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } وگرنه اگر (AstParamTypeDType* ثابت paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        تهی visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        تهی visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ثابت tdefp = nodep->typedefp();
            اگر (tdefp) {
                iterateTypedef(tdefp);
            } وگرنه اگر (AstParamTypeDType* ثابت paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        تهی visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ثابت varp = nodep->varp();
            اگر (firstReach(varp)) {
                iterate(varp->subDTypep());
                ثابت auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                اگر (varp->varType() == VVarType::LPARAM && deferredVarps.شمار(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    عمومی:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_گره* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

عمومی:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    تهی retargetIfaceRefs(Ast_گره_Module* parentModp, ثابت string& cellName) {
        Ast_گره_Module* ثابت correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        اگر (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) بازگشت;
        parentModp->foreach([&](AstRefDType* refp) {
            ثابت VIfaceCaptureTag* ثابت tagp = V3LinkDotIfaceCapture::captureTag(refp);
            اگر (!tagp || tagp->m_cellPath != cellName) بازگشت;
            // Only retarget REFDTYPEs owned by parentModp.
            اگر (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) بازگشت;
            اگر (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    تهی resolveDeferredDotsReachableFrom(Ast_گره* rootp, ثابت Ast_گره_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*این, rootp};
    }

    Ast_گره_Module* گره_Deparam(Ast_گره* nodep, Ast_گره_Module* srcModp, Ast_گره_Module* modp,
                               ثابت string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_گره::گره_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string گره_Name = nodep->name();
        اگر (AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            اگر (گره_Name.empty()) گره_Name = ifaceRefp->cellName();
        }
        ثابت string instanceName
            = گره_Name.empty() ? someInstanceName : (someInstanceName + "." + گره_Name);

        اگر (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            برای (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ثابت nextp = VN_AS(pinp->nextp(), Pin);
                اگر (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_درخت(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_گره::گره_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_گره_Module* newModp = nullptr;
        اگر (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } وگرنه اگر (AstIfaceRefDType* ثابت ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } وگرنه اگر (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وگرنه اگر (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وگرنه {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        اگر (!newModp) بازگشت nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_گره::گره_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        بازگشت newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        بازگشت taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        برای (Ast_گره_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گره_Module)) {
            m_allModuleNames.درج(modp->name());
        }
    }
    ~ParamProcessor() = پیشفرض;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

رده ClassRefUnlinkerVisitor final : عمومی VNVisitor {

عمومی:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    تهی visit(AstClassOrPackageRef* nodep) override {
        اگر (nodep->paramsp()) {
            اگر (AstClass* ثابت classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                اگر (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ثابت dotp = VN_CAST(nodep->backp(), Dot);
                    اگر (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(افزودن_Deletep(dotp), dotp, nodep);
                    } وگرنه {
                        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    تهی visit(AstClass* nodep) override {}  // don't iterate inside classes
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

رده ParamClassRefDTypeRelinkVisitor final : عمومی VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ثابت AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_گره_Module* m_ownerModp = nullptr;
    Ast_گره_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    تهی ensureOwnerMap() {
        اگر (m_ownerMapForp == m_ownerModp) بازگشت;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        اگر (!m_ownerModp) بازگشت;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        اگر (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            اگر (ownerClassp->hasGParam()) بازگشت;
        }
        برای (Ast_گره* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ثابت tdp = VN_CAST(sp, Typedef);
            اگر (!tdp) ادامه;
            AstClassRefDType* ثابت crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            اگر (!crdtp || !crdtp->classp()) ادامه;
            AstClass* ثابت targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            اگر (targetp->hasGParam()) ادامه;
            ثابت std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ثابت auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            اگر (!pair.second && pair.first->second != targetp) m_ambiguous.درج(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ایستا string classOrigName(ثابت AstClass* classp) {
        بازگشت classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ایستا AstTypedef* findTypedefWithBases(AstClass* classp, ثابت string& name) {
        برای (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            اگر (AstTypedef* ثابت tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                اگر (tdp->subDTypep()) بازگشت tdp;
            }
        }
        بازگشت nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    تهی retargetRefDType(AstRefDType* refp) {
        اگر (!m_ownerModp) بازگشت;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        اگر (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            اگر (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ثابت AstClass* refClassp = nullptr;
                اگر (AstTypedef* ثابت tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } وگرنه اگر (ثابت AstClassOrPackageRef* ثابت classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                اگر (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ثابت selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    اگر (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        بازگشت;
                    }
                }
            }
        }

        AstTypedef* ثابت oldTdp = refp->typedefp();
        اگر (!oldTdp) بازگشت;
        AstClass* ثابت oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        اگر (!oldOwnerp) بازگشت;

        ensureOwnerMap();
        اگر (m_origNameToClone.empty()) بازگشت;
        ثابت std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        اگر (m_ambiguous.شمار(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            بازگشت;
        }
        ثابت auto it = m_origNameToClone.find(origName);
        اگر (it == m_origNameToClone.end()) بازگشت;
        AstClass* ثابت correctClassp = it->second;
        اگر (correctClassp == oldOwnerp) بازگشت;
        AstTypedef* ثابت newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        اگر (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            بازگشت;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        اگر (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            بازگشت;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

عمومی:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        اگر (AstParamTypeDType* ثابت paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ثابت auto it = m_paramTypeClassMap.find(paramtypep);
            اگر (it != m_paramTypeClassMap.end()) {
                AstClass* ثابت origClassp = it->second;
                اگر (origClassp->hasGParam() && !origClassp->user3p()) {
                    اگر (AstClass* ثابت parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ثابت string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        برای (Ast_گره* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                اگر (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    بشکن;
                                }
                            }
                        }
                        اگر (newParamTypep) {
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
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

رده ParamState final {
عمومی:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<منطقی, صحیح>;
    std::multimap<WQKey, Ast_گره_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_گره_Module*, std::unordered_set<Ast_گره_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

رده ParamVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    منطقی m_iterateModule = نادرست;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<منطقی, Ast_گره*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ثابت Ast_گره*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ثابت AstRefDType*, منطقی>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ثابت Ast_گره_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    منطقی m_modIfaceRefsDone = نادرست;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    منطقی m_inGenerateCond = نادرست;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    تهی processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_گره_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = درست;

        // Visit all cells under module, recursively
        وقتی (درست) {
            // Classes specialized since the last pass still need their bodies elaborated
            برای (AstClass* ثابت classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{درست, classp->level()}, classp);
            }
            اگر (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            اگر (workQueue.empty()) بشکن;

            ثابت auto itm = workQueue.cbegin();
            Ast_گره_Module* ثابت modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            اگر (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                اگر (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = نادرست;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            وقتی (!m_cellps.empty()) {
                ثابت auto itim = m_cellps.cbegin();
                Ast_گره* ثابت cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ثابت auto genHierIt = m_genHierNames.find(cellp);
                اگر (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_گره_Module* srcModp = nullptr;
                اگر (ثابت AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } وگرنه اگر (ثابت AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    اگر (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ادامه;
                } وگرنه اگر (ثابت AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } وگرنه اگر (ثابت AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } وگرنه {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                اگر (!srcModp) ادامه;

                // Update path
                ثابت string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                اگر (Ast_گره_Module* ثابت newModp
                    = m_processor.گره_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    اگر (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        اگر (V3LinkDotIfaceCapture::enabled()) {
                            اگر (ثابت AstCell* ثابت modCellp = VN_CAST(cellp, Cell)) {
                                اگر (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].درج(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    اگر (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = نادرست;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ایستا string getRefBaseName(ثابت AstVarXRef* refp) {
        ثابت string dotted = refp->dotted();
        اگر (dotted.empty()) بازگشت "";
        بازگشت dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    تهی logTemplateLeakRefs(Ast_گره_Module* parentModp, Ast_گره_Module* templateModp,
                             ثابت نویسه* stage, Ast_گره* contextp) {
        اگر (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) بازگشت;
        // LCOV_EXCL_START  // Debug-only diagnostic
        صحیح leak_شمار = 0;
        ثابت auto ancestryOf = [](ثابت Ast_گره* nodep) {
            string ancestry;
            برای (ثابت Ast_گره* curp = nodep; curp; curp = curp->backp()) {
                اگر (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                اگر (VN_IS(curp, گره_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    بشکن;
                }
            }
            بازگشت ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            اگر (refp->typedefp()) {
                Ast_گره_Module* ثابت tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                اگر (tdOwnerp == templateModp) {
                    ++leak_شمار;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_گره::گره_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            اگر (refp->refDTypep()) {
                Ast_گره_Module* ثابت rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                اگر (rdOwnerp == templateModp) {
                    ++leak_شمار;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_گره::گره_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            اگر (!varrefp->varp()) بازگشت;
            Ast_گره_Module* ثابت varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            اگر (varOwnerp != templateModp) بازگشت;
            ++leak_شمار;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_گره::گره_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        اگر (leak_شمار > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_شمار);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    تهی checkParamNotHierRecurse(Ast_گره* nodep, منطقی underTypeQuery = نادرست) {
        برای (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ثابت AstAttrOf* ثابت attrp = VN_CAST(nodep, AttrOf);
            ثابت منطقی childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            اگر (ثابت AstVarXRef* ثابت refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                منطقی isIfaceRef = نادرست;
                اگر (refp->varp() && refp->varp()->isIfaceParam()) {
                    ثابت string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.شمار(refname) || m_ifaceInstCells.شمار(refname));
                }
                اگر (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } وگرنه اگر (ثابت Ast_گره_FTask_Ref* ثابت refp = VN_CAST(nodep, گره_FTask_Ref)) {
                اگر (refp->dotted() != "") {
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
    تهی specializeNestedIfaceCells(Ast_گره_Module* ifaceModp) {
        برای (Ast_گره* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت nestedCellp = VN_CAST(stmtp, Cell);
            اگر (!nestedCellp) ادامه;
            اگر (!VN_IS(nestedCellp->modp(), Iface)) ادامه;
            اگر (!nestedCellp->paramsp()) ادامه;

            Ast_گره_Module* ثابت nestedSrcModp = nestedCellp->modp();
            اگر (Ast_گره_Module* ثابت nestedNewModp = m_processor.گره_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                اگر (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    برای (Ast_گره* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        اگر (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                            اگر (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    اگر (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    منطقی cellParamsReferenceIfacePorts(AstCell* cellp) {
        اگر (!cellp->paramsp()) بازگشت نادرست;

        برای (Ast_گره* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            اگر (AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                اگر (Ast_گره* ثابت exprp = pinp->exprp()) {
                    اگر (ثابت AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
                        ثابت string refname = getRefBaseName(refp);
                        اگر (!refname.empty()
                            && (m_ifacePortNames.شمار(refname)
                                || m_ifaceInstCells.شمار(refname)))
                            بازگشت درست;
                    }
                }
            }
        }
        بازگشت نادرست;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    تهی expandIfaceArrayPortDimensions(AstVar* portp, Ast_گره_DType* dtypep,
                                        ثابت std::string& suffix) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        اگر (!arrp) {
            AstVar* ثابت varp = portp->clone_درخت(نادرست);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            اگر (Ast_گره_DType* ثابت oldp = varp->childDTypep()) {
                VL_DO_DANGLING(افزودن_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_درخت(نادرست));
            portp->addNextHere(varp);
            بازگشت;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        برای (ثابت صحیح n : arrp->declRange().seq_راست_To_چپ()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_گره::encode_عدد(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    تهی expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ثابت AstRange* rangep, ثابت std::string& suffix, صحیح idx) {
        // Base case: insert the element when no dimensions left
        اگر (!rangep) {
            AstCell* ثابت elemp = arrayedCellp->clone_درخت(نادرست);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            اگر (ifaceVarp) {
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_درخت(نادرست);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ثابت varp = ifaceVarp->clone_درخت(نادرست);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                اگر (Ast_گره_DType* ثابت oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(افزودن_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            بازگشت;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);
        برای (ثابت صحیح n : rangep->seq_راست_To_چپ()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_گره::encode_عدد(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    تهی visitCellOrClassRef(Ast_گره* nodep, منطقی isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        اگر (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ثابت cellp = VN_CAST(nodep, Cell);
            اگر (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_گره_Module* ثابت srcModp = cellp->modp();
                Ast_گره_Module* ثابت newModp
                    = m_processor.گره_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                اگر (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                اگر (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        اگر (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->recursiveClone()) nodep->dead(درست);  // Fake, made for recursive elimination
        اگر (nodep->dead()) بازگشت;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            اگر (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.افزودن_back(classp);
                بازگشت;
            }
        }

        اگر (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            بازگشت;
        }

        // Start traversal at root-like things
        اگر (nodep->is_بالاترین()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    منطقی isCircularType(ثابت AstRefDType* nodep) {
        ثابت auto iter = m_isCircular.emplace(nodep, درست);
        اگر (!iter.second) بازگشت iter.first->second;
        اگر (ثابت AstRefDType* ثابت subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ثابت منطقی ret = isCircularType(subDTypep);
            iter.first->second = ret;
            بازگشت ret;
        }
        iter.first->second = نادرست;
        بازگشت نادرست;
    }

    تهی visit(AstRefDType* nodep) override {
        ثابت منطقی isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        اگر (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        اگر (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } وگرنه اگر (nodep->typedefp() && nodep->subDTypep()
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
    تهی visit(AstCell* nodep) override {
        اگر (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            برای (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ثابت ifaceVarp = [&]() -> AstVar* {
                اگر (!VN_IS(nodep->modp(), Iface)) بازگشت nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ثابت varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                بازگشت varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            اگر (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(درست);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ثابت firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            بازگشت;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        اگر (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    تهی visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        اگر (nodep->ifacep()) visitCellOrClassRef(nodep, درست);
    }
    تهی visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, نادرست);
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        اگر (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, نادرست);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ایستا تهی gatherVars(Ast_گره* stmtsp, منطقی (*matchp)(ثابت AstVar*), VarsByName& varsr) {
        برای (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                اگر (matchp(varp)) varsr.emplace(varp->name(), varp);
            } وگرنه اگر (AstGenBlock* ثابت genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ثابت VarsByName& modIfaceRefs() {
        اگر (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ثابت AstVar* varp) { بازگشت varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = درست;
        }
        بازگشت m_modIfaceRefs;
    }
    ثابت VarsByName& ifaceParams(Ast_گره_Module* ifacep) {
        ثابت auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        اگر (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ثابت AstVar* varp) { بازگشت varp->isParam(); },
                pair.first->second);
        }
        بازگشت pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_گره* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ثابت lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ثابت rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        اگر (!lhsp || !rhsp) بازگشت nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        اگر (VL_UNCOVERABLE(!m_modp)) بازگشت nullptr;

        ثابت auto& refVars = modIfaceRefs();
        ثابت auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        اگر (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) بازگشت nullptr;
        AstVar* ثابت ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ثابت ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        اگر (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) بازگشت nullptr;

        ثابت auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ثابت auto targetVarIt = ifaceVars.find(rhsp->name());
        اگر (targetVarIt == ifaceVars.end()) بازگشت nullptr;
        AstVar* ثابت targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        اگر (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        اگر (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            بازگشت nullptr;  // LCOV_EXCL_LINE
        }
        بازگشت targetVarp->valuep()->clone_درخت(نادرست);
    }
    تهی resolveGenericIfaceDotsIn(Ast_گره* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.افزودن_back(dotp); });
        برای (AstDot* ثابت dotp : dotps) {
            اگر (Ast_گره* ثابت newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_درخت(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    تهی visit(AstVar* nodep) override {
        اگر (nodep->user2SetOnce()) بازگشت;  // Process once
        // Interface array port: split it into its elements
        اگر (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(درست);
        }
        // Build cache of interface port names as we encounter them
        اگر (nodep->isIfaceRef()) { m_ifacePortNames.درج(nodep->name()); }
        iterateChildren(nodep);
        اگر (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            اگر (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } وگرنه اگر (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                منطقی hasUnresolvedLparamXRef = نادرست;
                nodep->valuep()->foreach([&](ثابت AstVarXRef* xrefp) {
                    اگر (ثابت AstVar* ثابت varp = xrefp->varp()) {
                        اگر (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = درست;
                        }
                    }
                });
                اگر (hasUnresolvedLparamXRef) بازگشت;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ثابت منطقی hasDot = nodep->valuep()->exists([](AstDot*) { بازگشت درست; });
                منطقی refsDeferred = نادرست;
                اگر (!hasDot) {
                    ثابت auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ثابت AstVarRef* refp) {
                        اگر (refsDeferred) بازگشت;
                        AstVar* ثابت refVarp = refp->varp();
                        اگر (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.شمار(refVarp)) {
                            refsDeferred = درست;
                        }
                    });
                }
                اگر (hasDot || refsDeferred) {
                    v3Global.rootp()->افزودن_Deferred_Param_Varp(nodep);
                    بازگشت;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    تهی visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        اگر (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    تهی visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        اگر (nodep->varp()) iterate(nodep->varp());
    }
    منطقی ifaceParamReplace(AstVarXRef* nodep, Ast_گره* candp) {
        برای (; candp; candp = candp->nextp()) {
            اگر (nodep->name() == candp->name()) {
                اگر (AstVar* ثابت varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    اگر (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    بازگشت درست;
                } وگرنه اگر (ثابت AstPin* ثابت pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_درخت(نادرست));
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت درست;
                }
            }
        }
        بازگشت نادرست;
    }

    تهی deparamIfaceCellNow(AstCell* cellp) {
        اگر (!cellp->paramsp()) بازگشت;
        اگر (!VN_IS(cellp->modp(), Iface)) بازگشت;
        Ast_گره_Module* ثابت srcModp = cellp->modp();
        Ast_گره_Module* ثابت newModp
            = m_processor.گره_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        اگر (newModp && newModp != srcModp) {
            اگر (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        اگر (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    تهی visit(AstVarXRef* nodep) override {
        اگر (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            بازگشت;
        }
        // Parameters of an interface array are accessed through its first element
        اگر (nodep->varp() && nodep->varp()->isParam()) {
            ثابت auto it = m_ifaceInstCells.find(nodep->dotted());
            اگر (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ثابت string dotted = nodep->dotted();
        اگر (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ثابت Ast_گره* backp = nodep;
            وقتی ((backp = backp->backp())) {
                اگر (VN_IS(backp, گره_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    بشکن;
                }
                اگر (ثابت AstVar* ثابت varp = VN_CAST(backp, Var)) {
                    اگر (!varp->isIfaceRef()) ادامه;
                    // Through all dimensions of an array
                    ثابت AstIfaceRefDType* ثابت ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    اگر (!ifacerefp) ادامه;
                    // Interfaces passed in on the port map have ifaces
                    اگر (ثابت AstIface* ثابت ifacep = ifacerefp->ifacep()) {
                        اگر (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            اگر (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                بازگشت;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    وگرنه اگر (AstCell* ثابت cellp = ifacerefp->cellp()) {
                        اگر (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            اگر (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                بازگشت;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ثابت auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            اگر (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ثابت cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    تهی visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ثابت AstClass* lhsClassp = nullptr;
        ثابت AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        اگر (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_گره* rhsDefp = nullptr;
        AstClassOrPackageRef* ثابت rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        اگر (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        اگر (lhsClassp && rhsDefp) {
            m_state.m_dots.افزودن_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } وگرنه {
            iterate(nodep->rhsp());
        }
    }

    تهی visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ثابت varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_گره_FTask_Ref* ثابت taskrefp = VN_CAST(nodep->refp(), گره_FTask_Ref);
        AstEnumItemRef* ثابت enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        اگر (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } وگرنه اگر (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } وگرنه اگر (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } وگرنه {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            بازگشت;
        }
        iterate(nodep->cellrefp());

        اگر (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } وگرنه اگر (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } وگرنه {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        برای (Ast_گره *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<صحیح> indices;
        برای (Ast_گره* s = nodep->selp(); s; s = s->nextp()) {
            ثابت AstConst* ثابت constp = VN_CAST(s, Const);
            اگر (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                بازگشت;
            }
            indices.افزودن_back(constp->toSInt());
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
        اگر (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            بازگشت;
        }
        pos += baseName.length();
        برای (صحیح idx : indices) {
            ثابت string replacement = "__BRA__" + Ast_گره::encode_عدد(idx) + "__KET__";
            اگر (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                بازگشت;
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
    تهی iterateGenerateCond(Ast_گره* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = درست;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    تهی visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
            اگر (Ast_گره* ثابت keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } وگرنه {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Normal edit rules will now recurse the replacement
        } وگرنه {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    تهی visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        اگر (AstGenFor* ثابت forp = VN_AS(nodep->genforp(), GenFor)) {
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
            اگر (Ast_گره* ثابت stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } وگرنه {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    تهی visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    تهی visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        منطقی hit = نادرست;
        Ast_گره* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        برای (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            برای (Ast_گره* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ثابت AstConst* ثابت exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        برای (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            برای (Ast_گره* ep = itemp->condsp(); ep;) {
                Ast_گره* ثابت nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        برای (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            اگر (!itemp->isDefault()) {
                برای (Ast_گره* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    اگر (ثابت AstConst* ثابت ccondp = VN_CAST(ep, Const)) {
                        V3_عدد match{nodep, 1};
                        match.opEq(ccondp->رقم(), exprp->رقم());
                        اگر (!hit && match.isNeqZero()) {
                            hit = درست;
                            keepp = itemp->itemsp();
                        }
                    } وگرنه {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        برای (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            اگر (itemp->isDefault()) {
                اگر (!hit) {
                    hit = درست;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        اگر (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = پیشفرض;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

رده Param_بالاترین final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    تهی relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        برای (AstDot* ثابت dotp : m_state.m_dots) {
            ثابت AstClassOrPackageRef* ثابت classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ثابت AstClass* ثابت lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            برای (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    بشکن;
                }
            }
        }
    }

    تهی fixLevel(Ast_گره_Module* modp) {
        // Fix up level of module, based on who instantiates it
        اگر (modp->user1SetOnce()) بازگشت;  // Already fixed
        اگر (m_state.m_parentps[modp].empty()) بازگشت;  // Leave top levels alone
        صحیح maxParentLevel = 0;
        برای (Ast_گره_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        اگر (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    تهی resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_گره_Module*> modps;
        برای (Ast_گره_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), گره_Module);
            modp->unlinkFrBack();
            modps.افزودن_back(modp);
        }

        // Fix-up levels
        ثابت VNUser1InUse user1InUse;
        برای (Ast_گره_Module* ثابت modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ثابت Ast_گره_Module* ap, ثابت Ast_گره_Module* bp) {
                             بازگشت ap->level() < bp->level();
                         });

        // Re-insert modules
        برای (Ast_گره_Module* ثابت modp : modps) netlistp->addModulesp(modp);
    }

عمومی:
    // CONSTRUCTORS
    explicit Param_بالاترین(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ثابت ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ثابت ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_گره_FTask_Ref* ftaskrefp) {
            Ast_گره_FTask* ftaskp = ftaskrefp->taskp();
            اگر (!ftaskp || !ftaskp->classMethod()) بازگشت;
            ثابت std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ثابت auto ancestorClassOf = [](Ast_گره* startp) -> AstClass* {
                برای (Ast_گره* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    اگر (AstClass* ثابت cp = VN_CAST(np, Class)) بازگشت cp;
                }
                بازگشت nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            اگر (refClassp == ftaskrefp->classOrPackagep())
                بازگشت;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            اگر (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                بازگشت;
            }
            اگر (classp->user3p()) بازگشت;  // will not get removed, no need to relink
            AstClass* ثابت parametrizedClassp = VN_CAST(classp->user4p(), Class);
            اگر (!parametrizedClassp) بازگشت;
            Ast_گره_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_گره_FTask* ftaskp) {
                اگر (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    بازگشت درست;
                }
                بازگشت نادرست;
            });
            اگر (newFuncp) {
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
        برای (AstClass* ثابت classp : m_state.m_paramClasses) {
            اگر (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                اگر (V3Error::error_شمار()) ادامه;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(افزودن_Deletep(classp->unlinkFrBack()), classp);
            } وگرنه {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(نادرست);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_گره::user3_Clear_درخت();
        برای (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_گره* ثابت nodep) { nodep->user3(درست); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_گره* ثابت nodep) {
            nodep->foreachLink([&](Ast_گره** ثابت linkpp, ثابت نویسه* namep) {
                اگر (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_بالاترین() = پیشفرض;
    VL_UNCOPYABLE(Param_بالاترین);
};

//######################################################################
// Param class functions

تهی V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    اگر (dump_درخت_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_بالاترین{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    اگر (dump_درخت_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_درخت("param", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    برای (AstVar* ثابت varp : rootp->deferredParamVarps()) {
        اگر (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
