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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Param.h"

#அந்தர்பாவ "V3Case.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3HierBlock.h"
#அந்தர்பாவ "V3LinkDotIfaceCapture.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3Simulate.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3Unroll.h"
#அந்தர்பாவ "V3Width.h"

#அந்தர்பாவ <cctype>
#அந்தர்பாவ <cstdlib>
#அந்தர்பாவ <deque>
#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <set>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
நிலையான AstClassRefDType* class_Ref_DType_Of_முனை(Ast_முனை* nodep) {
    போது (AstTypedef* மாறிலி tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_முனை_DType* மாறிலி dtp = VN_CAST(nodep, முனை_DType);
    திரும்பு dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

வகுப்பு ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, மாறிலி V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<மாறிலி std::string, Ast_முனை_Module*>;
    using ParamConstMap = std::map<மாறிலி std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<மாறிலி std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    மாறிலி தருக்க m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<மாறிலி V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<மாறிலி std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<மாறிலி AstVar*> m_untypedParams;

    // METHODS

பொது:
    ParameterizedHierBlocks(மாறிலி V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        க்கு (மாறிலி auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            மாறிலி V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            க்கு (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    புதிய File_வரி{File_வரி::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                மாறிலி தருக்க inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.செருகு({hierOpt.second.origName(), {}});
        }
        க்கு (Ast_முனை_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), முனை_Module)) {
            எனில் (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            மாறிலி string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            மாறிலி auto defParamIt = m_modParams.find(actualModName);
            எனில் (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                        எனில் (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        மாறிலி AstBasicDType* மாறிலி dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        எனில் (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    தருக்க hierSubRun() மாறிலி { திரும்பு m_hierSubRun; }
    தருக்க isHierBlock(மாறிலி string& origName) மாறிலி {
        திரும்பு m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_முனை_Module* findByParams(மாறிலி string& origName, AstPin* firstPinp,
                                மாறிலி Ast_முனை_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        மாறிலி std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        மாறிலி auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        மாறிலி AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        க்கு (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            தருக்க found = உண்மை;
            size_t paramIdx = 0;
            மாறிலி ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            க்கு (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (!pinp->exprp()) தொடர்;
                எனில் (மாறிலி AstVar* மாறிலி modvarp = pinp->modVarp()) {
                    AstConst* மாறிலி constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    மாறிலி auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    மாறிலி தருக்க untyped = m_untypedParams.மொத்தம்(paramIt->second);
                    AstConst* மாறிலி defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    எனில் (defValuep && areSame(constp, defValuep)) {
                        எனில் (!untyped || constp->same_மதிப்பு_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            தொடர்;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    மாறிலி auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    எனில் (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = பொய்;
                        நிறுத்து;
                    }
                    எனில் (untyped && !constp->same_மதிப்பு_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = பொய்;
                        நிறுத்து;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            எனில் (found && paramIdx == hierIt->second->params().size()) நிறுத்து;
        }
        எனில் (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_முனை::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            திரும்பு nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        மாறிலி HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        மாறிலி auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        எனில் (it == m_hierBlockMod.end()) திரும்பு nullptr;
        திரும்பு it->second;
    }
    நிலையான தருக்க areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        எனில் (pinValuep->isString()) {
            திரும்பு pinValuep->இலக்கம்().toString() == hierOptParamp->இலக்கம்().toString();
        }

        எனில் (hierOptParamp->isDouble()) {
            இரட்டை var;
            எனில் (pinValuep->isDouble()) {
                var = pinValuep->இலக்கம்().toDouble();
            } இல்லையெனில் {  // Cast from integer to real
                V3_எண் var_இலக்கம்{pinValuep, V3_எண்::Double{}, 0.0};
                var_இலக்கம்.opIToRD(pinValuep->இலக்கம்());
                var = var_இலக்கம்.toDouble();
            }
            திரும்பு V3_எண்::epsilonEqual(var, hierOptParamp->இலக்கம்().toDouble());
        } இல்லையெனில் {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_எண் var_இலக்கம்{pinValuep, hierOptParamp->இலக்கம்().width()};
            எனில் (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_இலக்கம்.opRToIRoundS(pinValuep->இலக்கம்());
            } இல்லையெனில் எனில் (pinValuep->isSigned()) {
                var_இலக்கம்.opExtendS(pinValuep->இலக்கம்(), pinValuep->இலக்கம்().width());
            } இல்லையெனில் {
                var_இலக்கம்.opAssign(pinValuep->இலக்கம்());
            }
            V3_எண் isEq{pinValuep, 1};
            isEq.opEq(var_இலக்கம், hierOptParamp->இலக்கம்());
            திரும்பு isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

வகுப்பு ParamProcessor final {
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
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;
    மாறிலி VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<மாறிலி Ast_முனை*, Ast_முனை*>;
    கட்டமைப்பு ModInfo final {
        Ast_முனை_Module* மாறிலி m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_முனை_Module* modp)
            : m_modp{modp} {}
    };
    std::map<மாறிலி std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<மாறிலி std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<மாறிலி std::string, std::string> m_மதிப்பு_Names;  // Parameter value text to its name
    std::set<std::string> m_மதிப்பு_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    நிலையான constexpr std::string::size_type DIGEST_DIGITS = 8;

    மாறிலி Ast_முனை_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_மதிப்பு_Map = std::map<std::string, Ast_முனை*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_முனை_Module*, Default_மதிப்பு_Map> m_default_Parameter_மதிப்புகள்;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<மாறிலி Ast_முனை_Module*, மாறிலி AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, முழு>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, முழு> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<மாறிலி AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    நிலையான வெறுமை makeSmallNames(Ast_முனை_Module* modp) {
        std::vector<முழு> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isGParam() || varp->isIfaceRef()) {
                    எழுத்து ch = varp->name()[0];
                    ch = std::toupper(ch);
                    எனில் (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<முழு>(ch)] * 256 + ch);
                    usedLetter[static_cast<முழு>(ch)]++;
                }
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி typep = VN_CAST(stmtp, ParamTypeDType)) {
                மாறிலி எழுத்து ch = 'T';
                typep->user3(usedLetter[static_cast<முழு>(ch)] * 256 + ch);
                usedLetter[static_cast<முழு>(ch)]++;
            }
        }
    }
    நிலையான string paramSmallName(Ast_முனை_Module* modp, Ast_முனை* varp) {
        எனில் (varp->user3() <= 1) makeSmallNames(modp);
        முழு index = varp->user3() / 256;
        மாறிலி எழுத்து ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        போது (index) {
            st += cvtToStr(static_cast<எழுத்து>((index % 25) + 'A'));
            index /= 26;
        }
        திரும்பு st;
    }

    நிலையான தருக்க is_Aggregate_Param_மதிப்பு(மாறிலி Ast_முனை* nodep) {
        திரும்பு VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    நிலையான string param_மதிப்பு_String(மாறிலி Ast_முனை* nodep) {
        எனில் (மாறிலி AstRefDType* மாறிலி refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string சாவி = nodep->name();
        எனில் (மாறிலி AstIfaceRefDType* மாறிலி ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            எனில் (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                சாவி = ifrtp->cellp()->modp()->name();
            } இல்லையெனில் எனில் (ifrtp->ifacep()) {
                சாவி = ifrtp->ifacep()->name();
            } இல்லையெனில் {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி dtypep
                   = VN_CAST(nodep, முனை_UOr_Struct_DType)) {
            சாவி += " ";
            சாவி += dtypep->verilogKwd();
            சாவி += " {";
            க்கு (மாறிலி Ast_முனை* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                சாவி += param_மதிப்பு_String(memberp);
                சாவி += ";";
            }
            சாவி += "}";
        } இல்லையெனில் எனில் (மாறிலி AstMemberDType* மாறிலி dtypep = VN_CAST(nodep, MemberDType)) {
            சாவி += " ";
            சாவி += param_மதிப்பு_String(dtypep->subDTypep());
        } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி dtypep = VN_CAST(nodep, BasicDType)) {
            எனில் (dtypep->isSigned()) சாவி += " signed";
            எனில் (dtypep->isRanged()) {
                சாவி += "[" + cvtToStr(dtypep->இடது()) + ":" + cvtToStr(dtypep->வலது()) + "]";
            }
        } இல்லையெனில் எனில் (மாறிலி AstPackArrayDType* மாறிலி dtypep = VN_CAST(nodep, PackArrayDType)) {
            சாவி += "[";
            சாவி += cvtToStr(dtypep->இடது());
            சாவி += ":";
            சாவி += cvtToStr(dtypep->வலது());
            சாவி += "] ";
            சாவி += param_மதிப்பு_String(dtypep->subDTypep());
        } இல்லையெனில் எனில் (மாறிலி AstPattern* மாறிலி patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            சாவி += "'{";
            க்கு (மாறிலி AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                எனில் (memp->isDefault()) சாவி += "default";
                எனில் (மாறிலி AstText* மாறிலி textp = VN_CAST(memp->keyp(), Text)) {
                    சாவி += textp->text();
                } இல்லையெனில் எனில் (memp->keyp()) {
                    சாவி += param_மதிப்பு_String(memp->keyp());
                }
                சாவி += ":";
                எனில் (memp->repp()) சாவி += param_மதிப்பு_String(memp->repp()) + "x";
                க்கு (மாறிலி Ast_முனை_கோவை* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), முனை_கோவை)) {
                    சாவி += param_மதிப்பு_String(valuep) + ";";
                }
                சாவி += ",";
            }
            சாவி += "}";
        } இல்லையெனில் எனில் (மாறிலி AstConsArray* மாறிலி consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            சாவி += "{";
            க்கு (மாறிலி AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                சாவி += cvtToStr(itemp->index()) + ":" + param_மதிப்பு_String(itemp->valuep()) + ",";
            }
            எனில் (consArrayp->defaultp()) {
                சாவி += "default:" + param_மதிப்பு_String(consArrayp->defaultp()) + ",";
            }
            சாவி += "}";
        } இல்லையெனில் எனில் (மாறிலி AstConsStruct* மாறிலி consp = VN_CAST(nodep, ConsStruct)) {
            சாவி += "{";
            க்கு (மாறிலி AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                சாவி += param_மதிப்பு_String(itemp->valuep());
                சாவி += ",";
            }
            சாவி += "}";
        } இல்லையெனில் எனில் (மாறிலி AstClassRefDType* மாறிலி classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            எனில் (classRefp->classp()) {
                மாறிலி string& className = classRefp->classp()->name();
                மாறிலி string& origName = classRefp->classp()->origName();
                மாறிலி தருக்க isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                எனில் (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } இல்லையெனில் எனில் (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    சாவி = className;
                } இல்லையெனில் {
                    // Unspecialized class with no params - use origName
                    சாவி = origName;
                }
            } இல்லையெனில் {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            சாவி = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } இல்லையெனில் எனில் (மாறிலி AstQueueDType* மாறிலி dtypep = VN_CAST(nodep, QueueDType)) {
            சாவி = paramElemString(dtypep->subDTypep()) + "$[$";
            எனில் (dtypep->boundConst()) சாவி += ":" + cvtToStr(dtypep->boundConst());
            சாவி += "]";
        } இல்லையெனில் எனில் (மாறிலி AstDynArrayDType* மாறிலி dtypep = VN_CAST(nodep, DynArrayDType)) {
            சாவி = paramElemString(dtypep->subDTypep()) + "$[]";
        } இல்லையெனில் எனில் (மாறிலி AstAssocArrayDType* மாறிலி dtypep = VN_CAST(nodep, AssocArrayDType)) {
            சாவி = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->சாவி_DTypep()) + "]";
        } இல்லையெனில் எனில் (மாறிலி AstWildcardArrayDType* மாறிலி dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            சாவி = paramElemString(dtypep->subDTypep()) + "$[*]";
        } இல்லையெனில் எனில் (மாறிலி AstEnumDType* மாறிலி dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            சாவி += " enum {";
            க்கு (மாறிலி AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                மாறிலி AstConst* மாறிலி constp = VN_CAST(itemp->valuep(), Const);
                சாவி += itemp->name() + "=" + (constp ? constp->இலக்கம்().ascii() : "?") + ";";
            }
            சாவி += "}";
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = VN_CAST(nodep, முனை_DType)) {
            சாவி += dtypep->prettyDTypeName(உண்மை);
        }
        UASSERT_OBJ(!சாவி.empty(), nodep, "Parameter yielded no value string");
        திரும்பு சாவி;
    }
    // As paramValueString(), for the element or key type of a container
    நிலையான string paramElemString(மாறிலி Ast_முனை_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        மாறிலி AstClassRefDType* மாறிலி classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        எனில் (classRefp && classRefp->paramsp()) திரும்பு classRefp->prettyDTypeName(உண்மை);
        திரும்பு param_மதிப்பு_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    நிலையான string digestSuffix(மாறிலி string& text, std::set<string>& usedr) {
        மாறிலி string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        போது (digits < hex.size() && !usedr.செருகு(hex.substr(0, digits)).second) ++digits;
        திரும்பு hex.substr(0, digits);
    }
    string param_மதிப்பு_எண்(Ast_முனை* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        எனில் (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        மாறிலி string text = param_மதிப்பு_String(nodep);
        மாறிலி auto pair = m_மதிப்பு_Names.emplace(text, "");
        எனில் (pair.second) pair.first->second = "z" + digestSuffix(text, m_மதிப்பு_Suffixes);
        திரும்பு pair.first->second;
    }
    string moduleCalcName(மாறிலி Ast_முனை_Module* srcModp, மாறிலி string& longname) {
        string newname = longname;
        எனில் (longname.length() > 30) {
            மாறிலி auto pair = m_longMap.emplace(longname, "");
            எனில் (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        திரும்பு newname;
    }
    நிலையான Ast_முனை_DType* arraySubDTypep(Ast_முனை_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            திரும்பு adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        எனில் (மாறிலி AstBracketArrayDType* மாறிலி adtypep = VN_CAST(nodep, BracketArrayDType)) {
            திரும்பு adtypep->subDTypep();
        }
        திரும்பு nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    நிலையான Ast_முனை_DType* arraySubDTypeDeepp(Ast_முனை_DType* nodep) {
        போது (Ast_முனை_DType* மாறிலி subp = arraySubDTypep(nodep)) nodep = subp;
        திரும்பு nodep;
    }
    நிலையான தருக்க isString(Ast_முனை_DType* nodep) {
        எனில் (AstBasicDType* மாறிலி basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            திரும்பு basicp->isString();
        திரும்பு பொய்;
    }
    வெறுமை collectPins(CloneMap* clonemapp, Ast_முனை_Module* modp, தருக்க originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            மாறிலி Ast_முனை* originalParamp = nullptr;
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி ptp = VN_CAST(stmtp, ParamTypeDType)) {
                எனில் (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            எனில் (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            எனில் (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    வெறுமை relinkPins(மாறிலி CloneMap* clonemapp, AstPin* startpinp) {
        க்கு (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                மாறிலி auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } இல்லையெனில் எனில் (pinp->modPTypep()) {
                மாறிலி auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } இல்லையெனில் {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    வெறுமை relinkPinsByName(AstPin* startpinp, Ast_முனை_Module* modp) {
        std::map<மாறிலி string, AstVar*> nameToPin;
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        க்கு (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (மாறிலி AstVar* மாறிலி varp = pinp->modVarp()) {
                மாறிலி auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    நிலையான தருக்க hasDescendantDefparams(மாறிலி Ast_முனை_Module* modp,
                                       std::set<மாறிலி Ast_முனை_Module*>& visited) {
        எனில் (!visited.செருகு(modp).second) திரும்பு பொய்;
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* மாறிலி cellp = VN_CAST(stmtp, Cell);
            எனில் (!cellp) தொடர்;
            க்கு (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    திரும்பு உண்மை;
                }
            }
            எனில் (Ast_முனை_Module* மாறிலி childModp = cellp->modp()) {
                எனில் (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    திரும்பு உண்மை;
                }
            }
        }
        visited.erase(modp);
        திரும்பு பொய்;
    }
    நிலையான தருக்க hasDescendantDefparams(மாறிலி Ast_முனை_Module* modp) {
        std::set<மாறிலி Ast_முனை_Module*> visited;
        திரும்பு hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    மாறிலி AstPin* nestedDefparamp(மாறிலி Ast_முனை_Module* modp) {
        மாறிலி auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        எனில் (!pair.second) திரும்பு pair.first->second;  // Searched, or being searched
        மாறிலி AstPin* foundp = nullptr;
        modp->exists([&](மாறிலி AstCell* cellp) {
            க்கு (மாறிலி AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (!pinp->paramPath().empty()) foundp = pinp;
            }
            எனில் (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            திரும்பு foundp != nullptr;
        });
        pair.first->second = foundp;
        திரும்பு foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    வெறுமை checkSupportedParam(Ast_முனை_Module* modp, AstPin* pinp) மாறிலி {
        // ConsArray is not supported because that can not be set via -G
        // option.
        எனில் (pinp->modVarp()) {
            தருக்க supported = பொய்;
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(pinp->exprp(), Const)) {
                supported = உண்மை;
                எனில் (constp->இலக்கம்().isString()
                    && !V3HierBlock::stringParamPassable(constp->இலக்கம்().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            எனில் (!supported) {
                pinp->v3error(
                    Ast_முனை::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } இல்லையெனில் எனில் (மாறிலி AstParamTypeDType* மாறிலி typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_முனை::prettyNameQ(modp->origName()));
        }
    }
    தருக்க moduleExists(மாறிலி string& modName) மாறிலி {
        எனில் (m_allModuleNames.find(modName) != m_allModuleNames.end()) திரும்பு உண்மை;
        எனில் (m_modNameMap.find(modName) != m_modNameMap.end()) திரும்பு உண்மை;
        திரும்பு பொய்;
    }

    string parameterizedHierBlockName(Ast_முனை_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_முனை*> pins;

        AstPin* pinp = paramPinsp;
        போது (pinp) {
            checkSupportedParam(modp, pinp);
            எனில் (மாறிலி AstVar* மாறிலி varp = pinp->modVarp()) {
                எனில் (!pinp->exprp()) தொடர்;
                எனில் (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } இல்லையெனில் எனில் (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), முனை_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        மாறிலி auto pair = m_default_Parameter_மதிப்புகள்.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        எனில் (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            எனில் (மாறிலி AstPin* மாறிலி defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_முனை::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_மதிப்பு_Map params;
            க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                    எனில் (varp->isGParam()) {
                        AstConst* மாறிலி constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_முனை* மாறிலி dtypep = static_cast<Ast_முனை*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        மாறிலி auto paramsIt = pair.first;
        எனில் (paramsIt->second.empty()) திரும்பு modp->origName();  // modp has no parameter

        string longname = modp->origName();
        க்கு (auto&& default_மதிப்பு : paramsIt->second) {
            மாறிலி auto pinIt = pins.find(default_மதிப்பு.first);
            // If the pin does not have a value assigned, use the default one.
            மாறிலி Ast_முனை* மாறிலி nodep = pinIt == pins.end() ? default_மதிப்பு.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_மதிப்பு.first + "=";
            // constp can be nullptr

            எனில் (மாறிலி AstConst* மாறிலி p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->இலக்கம்().ascii(பொய்);
            } இல்லையெனில் எனில் (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_மரம்(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        மாறிலி auto iter = m_longMap.find(longname);
        எனில் (iter != m_longMap.end()) திரும்பு iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.செருகு(longname);
        போது (உண்மை) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            மாறிலி string hashStr = hashStrGen.digestHex();
            க்கு (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                எனில் (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                எனில் (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    திரும்பு newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.செருகு(V3Os::trueRandom(64));
        }
    }
    வெறுமை replaceRefsRecurse(Ast_முனை* மாறிலி nodep, மாறிலி AstClass* மாறிலி oldClassp,
                            AstClass* மாறிலி newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        எனில் (AstClassRefDType* மாறிலி classRefp = VN_CAST(nodep, ClassRefDType)) {
            எனில் (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } இல்லையெனில் எனில் (AstClassOrPackageRef* மாறிலி classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            எனில் (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } இல்லையெனில் எனில் (AstTypedef* மாறிலி typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            எனில் (typedefp->subDTypep()) {
                எனில் (AstClassRefDType* மாறிலி classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    எனில் (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } இல்லையெனில் எனில் (Ast_முனை_FTask_Ref* மாறிலி ftaskRefp = VN_CAST(nodep, முனை_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            எனில் (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            எனில் (Ast_முனை_FTask* மாறிலி oldTaskp = ftaskRefp->taskp()) {
                எனில் (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        எனில் (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        எனில் (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        எனில் (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        எனில் (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        எனில் (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    வகுப்பு VarXRefRelinkVisitor final : பொது VNVisitor {
        Ast_முனை_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_முனை_Module*> m_varModuleMap;  // Cache var->module lookups

    பொது:
        explicit VarXRefRelinkVisitor(Ast_முனை_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    தனிப்பட்ட:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_முனை_Module* findVarModule(AstVar* varp) {
            மாறிலி auto it = m_varModuleMap.find(varp);
            எனில் (it != m_varModuleMap.end()) திரும்பு it->second;
            Ast_முனை_Module* varModp = nullptr;
            க்கு (Ast_முனை* np = varp; np; np = np->backp()) {
                எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(np, முனை_Module)) {
                    varModp = modp;
                    நிறுத்து;
                }
            }
            m_varModuleMap[varp] = varModp;
            திரும்பு varModp;
        }

        வெறுமை visit(AstVarXRef* nodep) override {
            AstVar* மாறிலி varp = nodep->varp();
            எனில் (!varp) {
                iterateChildren(nodep);
                திரும்பு;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            மாறிலி string& dotted = nodep->dotted();
            எனில் (dotted.empty()) {
                iterateChildren(nodep);
                திரும்பு;
            }

            மாறிலி size_t dotPos = dotted.find('.');
            மாறிலி string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            எனில் (portName.empty()) {
                iterateChildren(nodep);
                திரும்பு;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            க்கு (Ast_முனை* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (AstVar* மாறிலி varChkp = VN_CAST(stmtp, Var)) {
                    எனில் (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        நிறுத்து;
                    }
                }
            }
            எனில் (!portVarp) {
                iterateChildren(nodep);
                திரும்பு;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* மாறிலி irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            எனில் (!irefp) {
                iterateChildren(nodep);
                திரும்பு;
            }

            Ast_முனை_Module* மாறிலி newIfacep = irefp->ifaceViaCellp();
            எனில் (!newIfacep) {
                iterateChildren(nodep);
                திரும்பு;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_முனை_Module* மாறிலி varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            எனில் (varModp && varModp != newIfacep) {
                க்கு (Ast_முனை* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (AstVar* மாறிலி newVarp = VN_CAST(stmtp, Var)) {
                        எனில் (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            நிறுத்து;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    வெறுமை fixupCrossModuleRefDTypes(Ast_முனை_Module* newModp) {
        எனில் (!V3LinkDotIfaceCapture::enabled()) திரும்பு;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        முழு fixed_மொத்தம் = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            மாறிலி VIfaceCaptureTag* மாறிலி tagp = V3LinkDotIfaceCapture::captureTag(refp);
            எனில் (!tagp) திரும்பு;

            Ast_முனை_Module* மாறிலி correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            எனில் (!correctModp || correctModp->dead()) திரும்பு;
            எனில் (correctModp->parameterizedTemplate()) திரும்பு;

            தருக்க fixed = பொய்;
            எனில் (refp->typedefp()) {
                எனில் (AstTypedef* மாறிலி newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    எனில் (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = உண்மை;
                }
            }
            எனில் (!fixed && refp->refDTypep()) {
                எனில் (Ast_முனை_DType* மாறிலி newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = உண்மை;
                }
            }
            எனில் (fixed) ++fixed_மொத்தம்;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_மொத்தம்);
    }

    // Return true on success, false on error
    தருக்க deepCloneModule(Ast_முனை_Module* srcModp, Ast_முனை* ifErrorp, AstPin* paramsp,
                         மாறிலி string& newname, மாறிலி IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_முனை_Module* newModp;
        எனில் (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_மரம்(பொய்), முனை_Module);
        } இல்லையெனில் {
            newModp = srcModp->clone_மரம்(பொய்);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        எனில் (AstIface* மாறிலி newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(உண்மை);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(பொய்);

        newModp->name(newname);
        newModp->user2(பொய்);  // We need to re-recurse this module once changed
        newModp->recursive(பொய்);
        newModp->recursiveClone(பொய்);
        newModp->hasGParam(பொய்);
        // Recursion may need level cleanups
        எனில் (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        எனில் ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_மரம்(), newModp);
            திரும்பு பொய்;
        }

        எனில் (AstClass* மாறிலி newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_முனை* insertp = srcModp;
        போது (VN_IS(insertp->nextp(), முனை_Module)
               && VN_AS(insertp->nextp(), முனை_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        மாறிலி auto iter = m_modNameMap.find(newname);
        CloneMap* மாறிலி clonemapp = &(iter->second.m_cloneMap);
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
        தருக்க needRelinkByName = பொய்;
        எனில் (paramsp) {
            க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = உண்மை;
                    நிறுத்து;
                }
            }
        }
        எனில் (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } இல்லையெனில் {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        க்கு (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            மாறிலி AstIfaceRefDType* மாறிலி portIrefp = it->first;
            மாறிலி AstIfaceRefDType* மாறிலி pinIrefp = it->second;
            AstIfaceRefDType* மாறிலி cloneIrefp = portIrefp->clonep();
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
        எனில் (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (pinp->exprp()) {
                எனில் (AstVar* மாறிலி modvarp = pinp->modVarp()) {
                    Ast_முனை* மாறிலி newp = pinp->exprp();  // Const or ConsArray
                    AstConst* மாறிலி exprp = VN_CAST(newp, Const);
                    AstConst* மாறிலி origp = VN_CAST(modvarp->valuep(), Const);
                    மாறிலி தருக்க overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    எனில் (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_மரம்();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_மரம்(பொய்));
                    modvarp->overriddenParam(overridden);
                } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி modptp = pinp->modPTypep()) {
                    Ast_முனை_DType* மாறிலி dtypep = VN_AS(pinp->exprp(), முனை_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    எனில் (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_மரம்();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_மரம்(பொய்));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        திரும்பு உண்மை;
    }
    மாறிலி ModInfo* moduleFindOrClone(Ast_முனை_Module* srcModp, Ast_முனை* ifErrorp, AstPin* paramsp,
                                     மாறிலி string& newname, மாறிலி IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        எனில் (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } இல்லையெனில் {
            எனில் (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                திரும்பு nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        மாறிலி ModInfo* மாறிலி modInfop = &(it->second);
        திரும்பு modInfop;
    }

    வெறுமை convertToStringp(Ast_முனை* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* மாறிலி constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        எனில் (constp && !constp->இலக்கம்().isString()) {
            constp->replaceWith(
                புதிய AstConst{constp->fileline(), AstConst::String{}, constp->இலக்கம்().toString()});
            VL_DO_DANGLING(constp->delete_மரம்(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    வெறுமை constify_Member_மதிப்பு(AstVar* varp, std::set<AstVar*>& inProgress) {
        எனில் (!varp->valuep() || VN_IS(varp->valuep(), Const)) திரும்பு;
        எனில் (!inProgress.செருகு(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_மரம்();
            varp->valuep(புதிய AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            திரும்பு;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](மாறிலி AstVarRef* refp) {
            AstVar* மாறிலி refVarp = refp->varp();
            எனில் (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.செருகு(refVarp);
            }
        });
        க்கு (AstVar* மாறிலி sibp : siblings) constify_Member_மதிப்பு(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    நிலையான AstParseRef* memberParseRef(Ast_முனை* nodep) {
        எனில் (AstParseRef* மாறிலி refp = VN_CAST(nodep, ParseRef)) திரும்பு refp;
        Ast_முனை_Pre_Sel* மாறிலி selp = VN_CAST(nodep, முனை_Pre_Sel);
        திரும்பு selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    நிலையான வெறுமை replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_முனை_கோவை* valuep) {
        எனில் (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } இல்லையெனில் {
            Ast_முனை_Pre_Sel* மாறிலி selp = VN_AS(dotp->rhsp(), முனை_Pre_Sel);
            எனில் (AstAttrOf* மாறிலி attrp = selp->attrp()) {
                எனில் (Ast_முனை* மாறிலி oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_மரம்(பொய்));
                    VL_DO_DANGLING(oldFromp->delete_மரம்(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_மரம்(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_மரம்(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    நிலையான தருக்க isDeferredMemberBase(மாறிலி Ast_முனை_கோவை* nodep) {
        எனில் (VN_IS(nodep, MemberSel)) திரும்பு உண்மை;
        எனில் (மாறிலி Ast_முனை_Pre_Sel* மாறிலி selp = VN_CAST(nodep, முனை_Pre_Sel)) {
            மாறிலி Ast_முனை_DType* மாறிலி fromDTypep = selp->fromp()->dtypep();
            மாறிலி Ast_முனை_Array_DType* மாறிலி arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), முனை_Array_DType) : nullptr;
            எனில் (arrayDTypep) {
                மாறிலி Ast_முனை_DType* மாறிலி elemDTypep = arrayDTypep->subDTypep();
                திரும்பு elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), முனை_UOr_Struct_DType);
            }
            திரும்பு isDeferredMemberBase(selp->fromp());
        }
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->dtypep();
        திரும்பு dtypep && VN_IS(dtypep->skipRefOrNullp(), முனை_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    நிலையான வெறுமை resolveMemberDot(AstDot* dotp) {
        Ast_முனை_கோவை* மாறிலி lhsp = VN_CAST(dotp->lhsp(), முனை_கோவை);
        AstParseRef* மாறிலி memberRefp = memberParseRef(dotp->rhsp());
        எனில் (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) திரும்பு;
        AstMemberSel* மாறிலி newp = புதிய AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    வெறுமை resolveDotToTypedef(Ast_முனை* exprp) {
        AstDot* மாறிலி dotp = VN_CAST(exprp, Dot);
        எனில் (!dotp) திரும்பு;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* மாறிலி classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        எனில் (!classRefp) {
            resolveMemberDot(dotp);
            திரும்பு;
        }
        AstParseRef* மாறிலி parseRefp = memberParseRef(dotp->rhsp());
        எனில் (!parseRefp) திரும்பு;

        மாறிலி AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* மாறிலி paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        எனில் (paramTypep) {
            Ast_முனை_DType* மாறிலி dtypep = paramTypep->subDTypep();
            AstClassRefDType* மாறிலி classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            எனில் (classRefDTypep) {
                AstClass* மாறிலி srcClassp = classRefDTypep->classp();
                எனில் (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    எனில் (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        எனில் (classRefp->paramsp()) {
            AstClass* மாறிலி srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            எனில் (srcClassp && srcClassp->hasGParam()) {
                எனில் (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        எனில் (AstClassRefDType* மாறிலி crp = class_Ref_DType_Of_முனை(classRefp->classOrPackageNodep())) {
            எனில் (AstClass* மாறிலி srcClassp = crp->classp()) {
                எனில் (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    எனில் (AstClass* மாறிலி newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        எனில் (!lhsClassp) திரும்பு;

        Ast_முனை* மாறிலி memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        எனில் (AstTypedef* மாறிலி tdefp = VN_CAST(memberp, Typedef)) {
            எனில் (parseRefp != dotp->rhsp()) திரும்பு;
            AstRefDType* மாறிலி refp = புதிய AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_மரம்(), dotp);
        } இல்லையெனில் எனில் (AstVar* மாறிலி varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    வெறுமை substituteParamMember(AstDot* மாறிலி dotp, AstParseRef* மாறிலி memberRefp,
                               AstVar* மாறிலி varp) {
        எனில் (!varp->isParam() || !varp->valuep()) திரும்பு;
        எனில் (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_மதிப்பு(varp, inProgress);
        }
        AstConst* மாறிலி constp = VN_CAST(varp->valuep(), Const);
        எனில் (!constp) திரும்பு;
        AstConst* மாறிலி newp = constp->clone_மரம்(பொய்);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    வெறுமை resolveParamClassRefDType(AstRefDType* refp) {
        எனில் (refp->typedefp() || refp->refDTypep()) திரும்பு;

        AstClassOrPackageRef* மாறிலி classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        எனில் (!classRefp) திரும்பு;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        எனில் (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            திரும்பு;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_முனை* targetp = classRefp->classOrPackageNodep();
        போது (மாறிலி AstTypedef* மாறிலி tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        எனில் (Ast_முனை_DType* மாறிலி dtargetp = VN_CAST(targetp, முனை_DType))
            targetp = dtargetp->skipRefOrNullp();
        எனில் (!targetp) திரும்பு;
        AstClassRefDType* மாறிலி classRefDTypep = VN_CAST(targetp, ClassRefDType);
        எனில் (!classRefDTypep) திரும்பு;
        srcClassp = classRefDTypep->classp();
        எனில் (!srcClassp) திரும்பு;

        AstClass* newClassp = srcClassp;
        எனில் (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* மாறிலி typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        எனில் (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    நிலையான தருக்க defaultParamsHaveTypeQueryOnParamType(மாறிலி AstClassRefDType* nodep) {
        தருக்க found = பொய்;
        nodep->foreach([&](மாறிலி AstAttrOf* attrp) {
            எனில் (found || !attrp->attrType().isTypeQuery()) திரும்பு;
            மாறிலி AstRefDType* மாறிலி refp = VN_CAST(attrp->fromp(), RefDType);
            எனில் (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = உண்மை;
        });
        திரும்பு found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    தருக்க classTypeMatchesDefaultClone(மாறிலி Ast_முனை_DType* exprp, மாறிலி Ast_முனை_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        மாறிலி auto* மாறிலி கோவை_Class_Refp = VN_CAST(exprp, ClassRefDType);
        மாறிலி auto* மாறிலி origClassRefp = VN_CAST(origp, ClassRefDType);
        எனில் (!கோவை_Class_Refp || !origClassRefp) திரும்பு பொய்;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        மாறிலி Ast_முனை_Module* மாறிலி defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        எனில் (defaultClonep && defaultClonep == கோவை_Class_Refp->classp()) திரும்பு உண்மை;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        எனில் (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) திரும்பு பொய்;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        எனில் (!origClassRefp->classp()->hasGParam()) திரும்பு பொய்;
        எனில் (origClassRefp->classp()->origName() != கோவை_Class_Refp->classp()->origName())
            திரும்பு பொய்;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        எனில் (!m_defaultCloneInProgress.செருகு(origClassRefp->classp()).second) திரும்பு பொய்;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* மாறிலி origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_மரம்(பொய்));
        Ast_முனை_Module* மாறிலி resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        மாறிலி தருக்க match = resolvedModp && VN_CAST(resolvedModp, Class) == கோவை_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_மரம்(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        திரும்பு match;
    }

    நிலையான தருக்க paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        எனில் (exprp->இலக்கம்().width() == origp->இலக்கம்().width()) திரும்பு பொய்;
        எனில் (exprp->இலக்கம்().isOpaque()) திரும்பு பொய்;
        எனில் (origp->இலக்கம்().isOpaque()) திரும்பு பொய்;
        மாறிலி முழு maxWidth = std::max(exprp->இலக்கம்().width(), origp->இலக்கம்().width());
        V3_எண் கோவை_இலக்கம்{exprp, maxWidth};
        எனில் (exprp->இலக்கம்().isSigned()) {
            கோவை_இலக்கம்.opExtendS(exprp->இலக்கம்(), exprp->இலக்கம்().width());
        } இல்லையெனில் {
            கோவை_இலக்கம்.opAssign(exprp->இலக்கம்());
        }
        V3_எண் orig_இலக்கம்{origp, maxWidth};
        எனில் (origp->இலக்கம்().isSigned()) {
            orig_இலக்கம்.opExtendS(origp->இலக்கம்(), origp->இலக்கம்().width());
        } இல்லையெனில் {
            orig_இலக்கம்.opAssign(origp->இலக்கம்());
        }
        V3_எண் isEq{exprp, 1};
        isEq.opEq(கோவை_இலக்கம், orig_இலக்கம்);
        திரும்பு isEq.isNeqZero();
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
    வெறுமை resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (!pinp->exprp()) தொடர்;
            எனில் (!VString::startsWith(pinp->name(), "__VGIfaceParam")) தொடர்;
            மாறிலி Ast_முனை_Var_Ref* மாறிலி fwdRefp = VN_CAST(pinp->exprp(), முனை_Var_Ref);
            எனில் (!fwdRefp) தொடர்;
            மாறிலி AstIfaceRefDType* மாறிலி resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* மாறிலி newIrefp = புதிய AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            எனில் (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_மரம்(உண்மை));
            }
            pinp->exprp()->unlinkFrBack()->delete_மரம்();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    வெறுமை name_By_Pin_மதிப்பு(AstPin* pinp, Ast_முனை_Module* srcModp, AstVar* modvarp, string& longnamer,
                        தருக்க& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_மதிப்பு_எண்(pinp->exprp());
        any_overridesr = உண்மை;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    வெறுமை cellPinCleanup(Ast_முனை* nodep, AstPin* pinp, AstPin* paramsp, Ast_முனை_Module* srcModp,
                        string& longnamer, தருக்க& any_overridesr,
                        std::unordered_set<மாறிலி AstPin*>& overridingTypePinsr) {
        எனில் (!pinp->exprp()) திரும்பு;  // No-connect
        எனில் (AstVar* மாறிலி modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            எனில் (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } இல்லையெனில் எனில் (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_மதிப்பு(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } இல்லையெனில் {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                எனில் (is_Aggregate_Param_மதிப்பு(pinp->exprp())) {
                    name_By_Pin_மதிப்பு(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    திரும்பு;
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
                எனில் (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    மாறிலி தருக்க dependent = modvarp->valuep()->exists(
                        [](Ast_முனை* np) { திரும்பு VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    எனில் (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                எனில் (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* மாறிலி exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* மாறிலி origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                எனில் (exprp && !exprp->இலக்கம்().isDouble() && !exprp->இலக்கம்().isString()) {
                    AstVar* cloneVarp = modvarp->clone_மரம்(பொய்);
                    தருக்க cloneVarpUnresolved = பொய்;
                    எனில் (Ast_முனை* மாறிலி oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_மரம்(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_மரம்(பொய்));
                    எனில் (Ast_முனை_DType* மாறிலி origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        எனில் (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_மரம்();
                        cloneVarp->childDTypep(origDTypep->clone_மரம்(பொய்));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr முழு maxSubstIters = 1000;
                        க்கு (முழு it = 0; it < maxSubstIters; ++it) {
                            தருக்க any = பொய்;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* மாறிலி targetp = varrefp->varp();
                                Ast_முனை* replacep = nullptr;
                                க்கு (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    எனில் (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        எனில் (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_மரம்(பொய்);
                                        }
                                        நிறுத்து;
                                    }
                                }
                                எனில் (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_மரம்(பொய்);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* மாறிலி patp = VN_CAST(replacep, Pattern);
                                எனில் (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_மரம்(பொய்));
                                }
                                எனில் (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_மரம்(), varrefp);
                                    any = உண்மை;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_முனை_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* மாறிலி ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                எனில் (!ptdp) திரும்பு;
                                AstPin* overridePinp = nullptr;
                                க்கு (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    எனில் (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        நிறுத்து;
                                    }
                                }
                                Ast_முனை_DType* மாறிலி substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), முனை_DType)
                                                   : ptdp->subDTypep();
                                எனில் (substp) toReplace.emplace_back(refp, substp);
                            });
                            க்கு (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* மாறிலி refp = it->first;
                                refp->replaceWith(it->second->clone_மரம்(பொய்));
                                VL_DO_DANGLING(refp->delete_மரம்(), refp);
                                any = உண்மை;
                            }
                            எனில் (!any) நிறுத்து;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_முனை* np) {
                            எனில் (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = உண்மை;
                                திரும்பு;
                            }
                            எனில் (AstEnumItemRef* மாறிலி itemRefp = VN_CAST(np, EnumItemRef)) {
                                எனில் (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = உண்மை;
                                }
                                திரும்பு;
                            }
                            AstRefDType* மாறிலி refp = VN_CAST(np, RefDType);
                            எனில் (!refp) திரும்பு;
                            எனில் (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = உண்மை;
                            }
                            // A type declared in the template must not be widthed there
                            எனில் (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = உண்மை;
                            }
                        });
                    }
                    எனில் (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        எனில் (AstConst* மாறிலி widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            எனில் (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_மரம்(), cloneVarp);
                }
                AstConst* மாறிலி namingExprp = normedNamep ? normedNamep : exprp;
                எனில் (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_முனை* மாறிலி pinExprp = pinp->exprp();
                    pinExprp->replaceWith(புதிய AstConst{pinp->fileline(), AstConst::Widthed_மதிப்பு{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_மரம்(), pinExprp);
                } இல்லையெனில் எனில் (origp
                           && (exprp->same_மரம்(origp)
                               || (exprp->இலக்கம்().width() == origp->இலக்கம்().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } இல்லையெனில் எனில் (namingExprp->இலக்கம்().isDouble() || namingExprp->இலக்கம்().isString()
                           || namingExprp->இலக்கம்().isFourState()
                           || namingExprp->இலக்கம்().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_மதிப்பு_எண்(namingExprp));
                    any_overridesr = உண்மை;
                } இல்லையெனில் {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->இலக்கம்().ascii(பொய்));
                    any_overridesr = உண்மை;
                }
                எனில் (normedNamep) VL_DO_DANGLING(normedNamep->delete_மரம்(), normedNamep);
            }
        } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_முனை_DType* rawTypep = VN_CAST(pinp->exprp(), முனை_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            தருக்க skipWidthForTemplateStruct = பொய்;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_முனை_DType* மாறிலி resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                எனில் (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_முனை_Module* மாறிலி ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    எனில் (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = உண்மை;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                எனில் (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_முனை_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            மாறிலி Ast_முனை_DType* origp = modvarp->skipRefToNonRefp();
            எனில் (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } இல்லையெனில் எனில் (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } இல்லையெனில் {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                எனில் (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), முனை_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                எனில் (!modvarp->fwdType().is_முனை_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                எனில் (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } இல்லையெனில் {
                    எனில் (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), முனை_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    எனில் (AstClassRefDType* மாறிலி classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        எனில் (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } இல்லையெனில் {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    வெறுமை cellInterfaceCleanup(AstPin* pinsp, Ast_முனை_Module* srcModp, string& longnamer,
                              தருக்க& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        க்கு (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            மாறிலி AstVar* மாறிலி modvarp = pinp->modVarp();
            எனில் (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                தொடர்;
            }
            எனில் (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* மாறிலி portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                மாறிலி Ast_முனை* மாறிலி exprp = pinp->exprp();
                எனில் (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    தொடர்;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி vrp = VN_CAST(exprp, முனை_Var_Ref)) {
                    எனில் (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                எனில் (!pinIrefp && exprp) {
                    மாறிலி Ast_முனை* basep = exprp;
                    போது (மாறிலி Ast_முனை_Pre_Sel* மாறிலி selp = VN_CAST(basep, முனை_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    எனில் (மாறிலி AstVarRef* மாறிலி vrp = VN_CAST(basep, VarRef)) {
                        எனில் (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                எனில் (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } இல்லையெனில் எனில் (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } இல்லையெனில் {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    எனில் (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_மதிப்பு_எண்(pinIrefp));
                        any_overridesr = உண்மை;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        எனில் (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_முனை::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_முனை::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    வெறுமை storeOriginalParams(Ast_முனை_Module* மாறிலி classp) {
        க்கு (Ast_முனை* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_முனை* originalParamp = nullptr;
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isGParam()) originalParamp = varp->clonep();
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி ptp = VN_CAST(stmtp, ParamTypeDType)) {
                எனில் (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            எனில் (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    வெறுமை genericInterfaceVarSetup(மாறிலி AstPin* மாறிலி paramsp, மாறிலி AstPin* மாறிலி pinsp) {
        std::unordered_map<string, மாறிலி AstPin*> paramspMap;
        க்கு (மாறிலி AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.செருகு({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        எனில் (paramspMap.empty()) திரும்பு;

        க்கு (மாறிலி Ast_முனை* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstPin* மாறிலி pinp = VN_CAST(nodep, Pin)) {
                எனில் (AstVar* மாறிலி varp = pinp->modVarp()) {
                    Ast_முனை_DType* dtypep = varp->childDTypep()->elemDTypep();
                    எனில் (AstIfaceGenericDType* மாறிலி ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        மாறிலி auto iter = paramspMap.find(varp->name());
                        எனில் (iter == paramspMap.end()) தொடர்;
                        Ast_முனை* மாறிலி backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        மாறிலி AstPin* மாறிலி paramp = iter->second;
                        paramspMap.erase(iter);
                        மாறிலி AstIfaceRefDType* மாறிலி ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* மாறிலி newIfacerefp = புதிய AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        எனில் (auto* மாறிலி arrDtp = VN_CAST(backp, முனை_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } இல்லையெனில் எனில் (auto* மாறிலி arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } இல்லையெனில் {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.தள்ளு_Deletep(ifaceGDTypep), ifaceGDTypep);
                        எனில் (paramspMap.empty()) திரும்பு;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    வெறுமை resolveDefaultParams(Ast_முனை* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        எனில் (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            எனில் (!classp) திரும்பு;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } இல்லையெனில் எனில் (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } இல்லையெனில் {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        க்கு (Ast_முனை* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                எனில் (!paramTypep->isGParam()) தொடர்;
                m_paramIndex.emplace(paramTypep, static_cast<முழு>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_முனை_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        க்கு (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* மாறிலி paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* மாறிலி reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            எனில் (!reqDtp) தொடர்;

            // If default is a reference to another param type, record dependency
            எனில் (AstRefDType* மாறிலி refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                எனில் (AstParamTypeDType* மாறிலி sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    எனில் (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    தொடர்;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            எனில் (Ast_முனை_DType* மாறிலி dtp = VN_CAST(reqDtp->lhsp(), முனை_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* மாறிலி typep = pinp->modPTypep();
            மாறிலி auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            எனில் (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } இல்லையெனில் {
                pinsByIndex.தள்ளு_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        க்கு (தருக்க resolvedSome = உண்மை; resolvedSome;) {
            resolvedSome = பொய்;
            க்கு (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                எனில் (pinsByIndex[paramIdx]) தொடர்;
                மாறிலி முழு sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                எனில் (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_மரம்(பொய்);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                எனில் (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_முனை_DType* மாறிலி dtypep = defaultTypeNodes[paramIdx];
                    newPinp = புதிய AstPin{dtypep->fileline(), static_cast<முழு>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_மரம்(பொய்)};
                }

                எனில் (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(உண்மை);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    எனில் (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } இல்லையெனில் எனில் (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    எனில் (!paramsp) paramsp = newPinp;
                    resolvedSome = உண்மை;
                }
            }
        }
    }

    Ast_முனை_Module* முனை_Deparam_Common(Ast_முனை* nodep, Ast_முனை_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, தருக்க any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        எனில் (debug() >= 9 && paramsp) paramsp->dump_மரம்_And_Next(cout, "-  cellparams: ");

        எனில் (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } இல்லையெனில் {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<மாறிலி AstPin*> overridingTypePins;
            க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (!pinp->exprp()) தொடர்;  // An empty override keeps the default
                எனில் (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    தொடர்;
                }
                AstVar* மாறிலி modvarp = pinp->modVarp();
                எனில் (!modvarp || !modvarp->isGParam()) தொடர்;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                மாறிலி AstPattern* மாறிலி patternp = VN_CAST(pinp->exprp(), Pattern);
                எனில் (patternp && !patternp->childDTypep()) தொடர்;
                எனில் (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_மதிப்பு(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (AstParamTypeDType* மாறிலி modptp = pinp->modPTypep()) {
                    எனில் (!overridingTypePins.மொத்தம்(pinp)) தொடர்;
                    Ast_முனை_DType* மாறிலி typep = VN_AS(pinp->exprp(), முனை_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_மதிப்பு_எண்(typep->skipRefToNonRefp());
                    any_overrides = உண்மை;
                    தொடர்;
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
        தருக்க defaultsResolved = பொய்;
        எனில் (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            க்கு (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                எனில் (pinp->modPTypep()) {
                    any_overrides = உண்மை;
                    defaultsResolved = உண்மை;
                    நிறுத்து;
                }
            }
        }
        எனில் (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = உண்மை;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_முனை_Module* newModp = nullptr;
        எனில் (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_முனை_Module* மாறிலி paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            எனில் (!paramedModp) திரும்பு nullptr;  // Unsupported, already reported
            paramedModp->dead(பொய்);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } இல்லையெனில் எனில் (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            எனில் (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_முனை_Module* முனை_Copyp = srcModp->clone_மரம்(பொய்);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << முனை_Copyp);
                m_deleter.தள்ளு_Deletep(முனை_Copyp);  // nodeCopyp used past here
                srcModp->user3p(முனை_Copyp);
                storeOriginalParams(முனை_Copyp);
            }
            newModp = srcModp;
        } இல்லையெனில் {
            மாறிலி string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            மாறிலி ModInfo* மாறிலி modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            எனில் (!modInfop) திரும்பு nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        மாறிலி தருக்க cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* மாறிலி newClassp = VN_CAST(newModp, Class);
        எனில் (newClassp && !newClassp->user2()) m_specializedClassps.தள்ளு_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        எனில் (defaultsResolved) srcModp->user4p(newModp);

        க்கு (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                எனில் (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        எனில் (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_மரம்(), paramsp);
        திரும்பு newModp;
    }

    Ast_முனை_Module* cellDeparam(AstCell* nodep, Ast_முனை_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_முனை_Module* மாறிலி newModp = முனை_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        எனில் (!newModp) திரும்பு nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(பொய்);
        திரும்பு newModp;
    }
    Ast_முனை_Module* ifaceRefDeparam(AstIfaceRefDType* மாறிலி nodep, Ast_முனை_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        எனில் (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            மாறிலி string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            மாறிலி string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            மாறிலி size_t ownerPos = ownerBaseName.find("__");
            எனில் (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            மாறிலி size_t srcPos = srcBaseName.find("__");
            எனில் (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            எனில் (ownerBaseName == srcBaseName) {
                தருக்க allOwnParams = உண்மை;
                க்கு (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    எனில் (AstRefDType* மாறிலி refp = VN_CAST(pinp->exprp(), RefDType)) {
                        எனில் (AstParamTypeDType* மாறிலி ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_முனை_Module* மாறிலி ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            எனில் (ptdOwnerp != m_modp) allOwnParams = பொய்;
                        } இல்லையெனில் {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } இல்லையெனில் {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                எனில் (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    எனில் (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_மரம்();
                    திரும்பு ownerIfacep;
                }
            }
        }

        Ast_முனை_Module* மாறிலி newModp
            = முனை_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, பொய்);
        எனில் (!newModp) திரும்பு nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        திரும்பு newModp;
    }
    Ast_முனை_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_முனை_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_முனை_Module* மாறிலி newModp
            = முனை_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, பொய்);
        எனில் (!newModp) திரும்பு nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* மாறிலி refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* மாறிலி newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        எனில் (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            எனில் (AstTypedef* மாறிலி typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        திரும்பு newModp;
    }
    Ast_முனை_Module* classRefDeparam(AstClassRefDType* nodep, Ast_முனை_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_முனை_Module* மாறிலி newModp
            = முனை_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, பொய்);
        எனில் (!newModp) திரும்பு nullptr;
        AstClass* மாறிலி newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        திரும்பு newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    வகுப்பு DeferredResolverVisitor final : பொது VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<மாறிலி Ast_முனை*> m_reachedDecls;

        தருக்க firstReach(மாறிலி Ast_முனை* மாறிலி declp) { திரும்பு m_reachedDecls.செருகு(declp).second; }

        வெறுமை iterateParamType(AstParamTypeDType* மாறிலி paramTypep) {
            எனில் (firstReach(paramTypep)) iterate(paramTypep);
        }
        வெறுமை iterateTypedef(AstTypedef* மாறிலி tdefp) {
            எனில் (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
        வெறுமை visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_முனை* மாறிலி targetp = nodep->classOrPackageNodep();
            எனில் (AstTypedef* மாறிலி tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        வெறுமை visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        வெறுமை visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* மாறிலி tdefp = nodep->typedefp();
            எனில் (tdefp) {
                iterateTypedef(tdefp);
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        வெறுமை visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* மாறிலி varp = nodep->varp();
            எனில் (firstReach(varp)) {
                iterate(varp->subDTypep());
                மாறிலி auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                எனில் (varp->varType() == VVarType::LPARAM && deferredVarps.மொத்தம்(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    பொது:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_முனை* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

பொது:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    வெறுமை retargetIfaceRefs(Ast_முனை_Module* parentModp, மாறிலி string& cellName) {
        Ast_முனை_Module* மாறிலி correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        எனில் (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) திரும்பு;
        parentModp->foreach([&](AstRefDType* refp) {
            மாறிலி VIfaceCaptureTag* மாறிலி tagp = V3LinkDotIfaceCapture::captureTag(refp);
            எனில் (!tagp || tagp->m_cellPath != cellName) திரும்பு;
            // Only retarget REFDTYPEs owned by parentModp.
            எனில் (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) திரும்பு;
            எனில் (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    வெறுமை resolveDeferredDotsReachableFrom(Ast_முனை* rootp, மாறிலி Ast_முனை_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*இது, rootp};
    }

    Ast_முனை_Module* முனை_Deparam(Ast_முனை* nodep, Ast_முனை_Module* srcModp, Ast_முனை_Module* modp,
                               மாறிலி string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_முனை::முனை_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string முனை_Name = nodep->name();
        எனில் (AstIfaceRefDType* மாறிலி ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            எனில் (முனை_Name.empty()) முனை_Name = ifaceRefp->cellName();
        }
        மாறிலி string instanceName
            = முனை_Name.empty() ? someInstanceName : (someInstanceName + "." + முனை_Name);

        எனில் (AstCell* மாறிலி cellp = VN_CAST(nodep, Cell)) {
            க்கு (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* மாறிலி nextp = VN_AS(pinp->nextp(), Pin);
                எனில் (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_மரம்(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_முனை::முனை_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_முனை_Module* newModp = nullptr;
        எனில் (AstCell* மாறிலி cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } இல்லையெனில் எனில் (AstIfaceRefDType* மாறிலி ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } இல்லையெனில் எனில் (AstClassRefDType* மாறிலி classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } இல்லையெனில் எனில் (AstClassOrPackageRef* மாறிலி classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } இல்லையெனில் {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        எனில் (!newModp) திரும்பு nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_முனை::முனை_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        திரும்பு newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        திரும்பு taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        க்கு (Ast_முனை_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), முனை_Module)) {
            m_allModuleNames.செருகு(modp->name());
        }
    }
    ~ParamProcessor() = இயல்பு;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

வகுப்பு ClassRefUnlinkerVisitor final : பொது VNVisitor {

பொது:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        எனில் (nodep->paramsp()) {
            எனில் (AstClass* மாறிலி classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                எனில் (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* மாறிலி dotp = VN_CAST(nodep->backp(), Dot);
                    எனில் (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(தள்ளு_Deletep(dotp), dotp, nodep);
                    } இல்லையெனில் {
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    வெறுமை visit(AstClass* nodep) override {}  // don't iterate inside classes
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

வகுப்பு ParamClassRefDTypeRelinkVisitor final : பொது VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<மாறிலி AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_முனை_Module* m_ownerModp = nullptr;
    Ast_முனை_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    வெறுமை ensureOwnerMap() {
        எனில் (m_ownerMapForp == m_ownerModp) திரும்பு;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        எனில் (!m_ownerModp) திரும்பு;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        எனில் (AstClass* மாறிலி ownerClassp = VN_CAST(m_ownerModp, Class)) {
            எனில் (ownerClassp->hasGParam()) திரும்பு;
        }
        க்கு (Ast_முனை* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* மாறிலி tdp = VN_CAST(sp, Typedef);
            எனில் (!tdp) தொடர்;
            AstClassRefDType* மாறிலி crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            எனில் (!crdtp || !crdtp->classp()) தொடர்;
            AstClass* மாறிலி targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            எனில் (targetp->hasGParam()) தொடர்;
            மாறிலி std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            மாறிலி auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            எனில் (!pair.second && pair.first->second != targetp) m_ambiguous.செருகு(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    நிலையான string classOrigName(மாறிலி AstClass* classp) {
        திரும்பு classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    நிலையான AstTypedef* findTypedefWithBases(AstClass* classp, மாறிலி string& name) {
        க்கு (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            எனில் (AstTypedef* மாறிலி tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                எனில் (tdp->subDTypep()) திரும்பு tdp;
            }
        }
        திரும்பு nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    வெறுமை retargetRefDType(AstRefDType* refp) {
        எனில் (!m_ownerModp) திரும்பு;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        எனில் (AstClass* மாறிலி ownerClassp = VN_CAST(m_ownerModp, Class)) {
            எனில் (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                மாறிலி AstClass* refClassp = nullptr;
                எனில் (AstTypedef* மாறிலி tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } இல்லையெனில் எனில் (மாறிலி AstClassOrPackageRef* மாறிலி classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                எனில் (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* மாறிலி selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    எனில் (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        திரும்பு;
                    }
                }
            }
        }

        AstTypedef* மாறிலி oldTdp = refp->typedefp();
        எனில் (!oldTdp) திரும்பு;
        AstClass* மாறிலி oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        எனில் (!oldOwnerp) திரும்பு;

        ensureOwnerMap();
        எனில் (m_origNameToClone.empty()) திரும்பு;
        மாறிலி std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        எனில் (m_ambiguous.மொத்தம்(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            திரும்பு;
        }
        மாறிலி auto it = m_origNameToClone.find(origName);
        எனில் (it == m_origNameToClone.end()) திரும்பு;
        AstClass* மாறிலி correctClassp = it->second;
        எனில் (correctClassp == oldOwnerp) திரும்பு;
        AstTypedef* மாறிலி newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        எனில் (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            திரும்பு;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        எனில் (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            திரும்பு;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

பொது:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstParamTypeDType* மாறிலி ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        எனில் (AstParamTypeDType* மாறிலி paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            மாறிலி auto it = m_paramTypeClassMap.find(paramtypep);
            எனில் (it != m_paramTypeClassMap.end()) {
                AstClass* மாறிலி origClassp = it->second;
                எனில் (origClassp->hasGParam() && !origClassp->user3p()) {
                    எனில் (AstClass* மாறிலி parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        மாறிலி string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        க்கு (Ast_முனை* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            எனில் (AstParamTypeDType* மாறிலி ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                எனில் (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    நிறுத்து;
                                }
                            }
                        }
                        எனில் (newParamTypep) {
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
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

வகுப்பு ParamState final {
பொது:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<தருக்க, முழு>;
    std::multimap<WQKey, Ast_முனை_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_முனை_Module*, std::unordered_set<Ast_முனை_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

வகுப்பு ParamVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    தருக்க m_iterateModule = பொய்;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<தருக்க, Ast_முனை*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<மாறிலி Ast_முனை*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<மாறிலி AstRefDType*, தருக்க>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<மாறிலி Ast_முனை_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    தருக்க m_modIfaceRefsDone = பொய்;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    தருக்க m_inGenerateCond = பொய்;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    வெறுமை processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_முனை_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = உண்மை;

        // Visit all cells under module, recursively
        போது (உண்மை) {
            // Classes specialized since the last pass still need their bodies elaborated
            க்கு (AstClass* மாறிலி classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{உண்மை, classp->level()}, classp);
            }
            எனில் (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            எனில் (workQueue.empty()) நிறுத்து;

            மாறிலி auto itm = workQueue.cbegin();
            Ast_முனை_Module* மாறிலி modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            எனில் (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                எனில் (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = பொய்;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            போது (!m_cellps.empty()) {
                மாறிலி auto itim = m_cellps.cbegin();
                Ast_முனை* மாறிலி cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                மாறிலி auto genHierIt = m_genHierNames.find(cellp);
                எனில் (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_முனை_Module* srcModp = nullptr;
                எனில் (மாறிலி AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } இல்லையெனில் எனில் (மாறிலி AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    எனில் (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) தொடர்;
                } இல்லையெனில் எனில் (மாறிலி AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } இல்லையெனில் எனில் (மாறிலி AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } இல்லையெனில் {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                எனில் (!srcModp) தொடர்;

                // Update path
                மாறிலி string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                எனில் (Ast_முனை_Module* மாறிலி newModp
                    = m_processor.முனை_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    எனில் (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        எனில் (V3LinkDotIfaceCapture::enabled()) {
                            எனில் (மாறிலி AstCell* மாறிலி modCellp = VN_CAST(cellp, Cell)) {
                                எனில் (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].செருகு(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    எனில் (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = பொய்;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    நிலையான string getRefBaseName(மாறிலி AstVarXRef* refp) {
        மாறிலி string dotted = refp->dotted();
        எனில் (dotted.empty()) திரும்பு "";
        திரும்பு dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    வெறுமை logTemplateLeakRefs(Ast_முனை_Module* parentModp, Ast_முனை_Module* templateModp,
                             மாறிலி எழுத்து* stage, Ast_முனை* contextp) {
        எனில் (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) திரும்பு;
        // LCOV_EXCL_START  // Debug-only diagnostic
        முழு leak_மொத்தம் = 0;
        மாறிலி auto ancestryOf = [](மாறிலி Ast_முனை* nodep) {
            string ancestry;
            க்கு (மாறிலி Ast_முனை* curp = nodep; curp; curp = curp->backp()) {
                எனில் (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                எனில் (VN_IS(curp, முனை_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    நிறுத்து;
                }
            }
            திரும்பு ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            எனில் (refp->typedefp()) {
                Ast_முனை_Module* மாறிலி tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                எனில் (tdOwnerp == templateModp) {
                    ++leak_மொத்தம்;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_முனை::முனை_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            எனில் (refp->refDTypep()) {
                Ast_முனை_Module* மாறிலி rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                எனில் (rdOwnerp == templateModp) {
                    ++leak_மொத்தம்;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_முனை::முனை_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            எனில் (!varrefp->varp()) திரும்பு;
            Ast_முனை_Module* மாறிலி varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            எனில் (varOwnerp != templateModp) திரும்பு;
            ++leak_மொத்தம்;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_முனை::முனை_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        எனில் (leak_மொத்தம் > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_மொத்தம்);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    வெறுமை checkParamNotHierRecurse(Ast_முனை* nodep, தருக்க underTypeQuery = பொய்) {
        க்கு (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            மாறிலி AstAttrOf* மாறிலி attrp = VN_CAST(nodep, AttrOf);
            மாறிலி தருக்க childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            எனில் (மாறிலி AstVarXRef* மாறிலி refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                தருக்க isIfaceRef = பொய்;
                எனில் (refp->varp() && refp->varp()->isIfaceParam()) {
                    மாறிலி string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.மொத்தம்(refname) || m_ifaceInstCells.மொத்தம்(refname));
                }
                எனில் (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } இல்லையெனில் எனில் (மாறிலி Ast_முனை_FTask_Ref* மாறிலி refp = VN_CAST(nodep, முனை_FTask_Ref)) {
                எனில் (refp->dotted() != "") {
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
    வெறுமை specializeNestedIfaceCells(Ast_முனை_Module* ifaceModp) {
        க்கு (Ast_முனை* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* மாறிலி nestedCellp = VN_CAST(stmtp, Cell);
            எனில் (!nestedCellp) தொடர்;
            எனில் (!VN_IS(nestedCellp->modp(), Iface)) தொடர்;
            எனில் (!nestedCellp->paramsp()) தொடர்;

            Ast_முனை_Module* மாறிலி nestedSrcModp = nestedCellp->modp();
            எனில் (Ast_முனை_Module* மாறிலி nestedNewModp = m_processor.முனை_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                எனில் (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    க்கு (Ast_முனை* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        எனில் (AstVar* மாறிலி varp = VN_CAST(sp, Var)) {
                            எனில் (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    எனில் (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    தருக்க cellParamsReferenceIfacePorts(AstCell* cellp) {
        எனில் (!cellp->paramsp()) திரும்பு பொய்;

        க்கு (Ast_முனை* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            எனில் (AstPin* மாறிலி pinp = VN_CAST(nodep, Pin)) {
                எனில் (Ast_முனை* மாறிலி exprp = pinp->exprp()) {
                    எனில் (மாறிலி AstVarXRef* மாறிலி refp = VN_CAST(exprp, VarXRef)) {
                        மாறிலி string refname = getRefBaseName(refp);
                        எனில் (!refname.empty()
                            && (m_ifacePortNames.மொத்தம்(refname)
                                || m_ifaceInstCells.மொத்தம்(refname)))
                            திரும்பு உண்மை;
                    }
                }
            }
        }
        திரும்பு பொய்;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    வெறுமை expandIfaceArrayPortDimensions(AstVar* portp, Ast_முனை_DType* dtypep,
                                        மாறிலி std::string& suffix) {
        மாறிலி AstUnpackArrayDType* மாறிலி arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        எனில் (!arrp) {
            AstVar* மாறிலி varp = portp->clone_மரம்(பொய்);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            எனில் (Ast_முனை_DType* மாறிலி oldp = varp->childDTypep()) {
                VL_DO_DANGLING(தள்ளு_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_மரம்(பொய்));
            portp->addNextHere(varp);
            திரும்பு;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        க்கு (மாறிலி முழு n : arrp->declRange().seq_வலது_To_இடது()) {
            மாறிலி std::string s = suffix + "__BRA__" + Ast_முனை::encode_எண்(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    வெறுமை expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   மாறிலி AstRange* rangep, மாறிலி std::string& suffix, முழு idx) {
        // Base case: insert the element when no dimensions left
        எனில் (!rangep) {
            AstCell* மாறிலி elemp = arrayedCellp->clone_மரம்(பொய்);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            எனில் (ifaceVarp) {
                AstIfaceRefDType* மாறிலி irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_மரம்(பொய்);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* மாறிலி varp = ifaceVarp->clone_மரம்(பொய்);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                எனில் (Ast_முனை_DType* மாறிலி oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(தள்ளு_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            திரும்பு;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        மாறிலி AstRange* மாறிலி subRangep = VN_AS(rangep->nextp(), Range);
        க்கு (மாறிலி முழு n : rangep->seq_வலது_To_இடது()) {
            மாறிலி std::string s = suffix + "__BRA__" + Ast_முனை::encode_எண்(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    வெறுமை visitCellOrClassRef(Ast_முனை* nodep, தருக்க isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        எனில் (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* மாறிலி cellp = VN_CAST(nodep, Cell);
            எனில் (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_முனை_Module* மாறிலி srcModp = cellp->modp();
                Ast_முனை_Module* மாறிலி newModp
                    = m_processor.முனை_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                எனில் (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                எனில் (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        எனில் (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->recursiveClone()) nodep->dead(உண்மை);  // Fake, made for recursive elimination
        எனில் (nodep->dead()) திரும்பு;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            எனில் (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.தள்ளு_back(classp);
                திரும்பு;
            }
        }

        எனில் (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            திரும்பு;
        }

        // Start traversal at root-like things
        எனில் (nodep->is_உச்சி()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    தருக்க isCircularType(மாறிலி AstRefDType* nodep) {
        மாறிலி auto iter = m_isCircular.emplace(nodep, உண்மை);
        எனில் (!iter.second) திரும்பு iter.first->second;
        எனில் (மாறிலி AstRefDType* மாறிலி subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            மாறிலி தருக்க ret = isCircularType(subDTypep);
            iter.first->second = ret;
            திரும்பு ret;
        }
        iter.first->second = பொய்;
        திரும்பு பொய்;
    }

    வெறுமை visit(AstRefDType* nodep) override {
        மாறிலி தருக்க isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        எனில் (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        எனில் (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } இல்லையெனில் எனில் (nodep->typedefp() && nodep->subDTypep()
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
    வெறுமை visit(AstCell* nodep) override {
        எனில் (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            க்கு (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* மாறிலி ifaceVarp = [&]() -> AstVar* {
                எனில் (!VN_IS(nodep->modp(), Iface)) திரும்பு nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* மாறிலி varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                திரும்பு varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            எனில் (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(உண்மை);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* மாறிலி firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* மாறிலி irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            திரும்பு;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        எனில் (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    வெறுமை visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        எனில் (nodep->ifacep()) visitCellOrClassRef(nodep, உண்மை);
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, பொய்);
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        எனில் (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, பொய்);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    நிலையான வெறுமை gatherVars(Ast_முனை* stmtsp, தருக்க (*matchp)(மாறிலி AstVar*), VarsByName& varsr) {
        க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                எனில் (matchp(varp)) varsr.emplace(varp->name(), varp);
            } இல்லையெனில் எனில் (AstGenBlock* மாறிலி genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    மாறிலி VarsByName& modIfaceRefs() {
        எனில் (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](மாறிலி AstVar* varp) { திரும்பு varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = உண்மை;
        }
        திரும்பு m_modIfaceRefs;
    }
    மாறிலி VarsByName& ifaceParams(Ast_முனை_Module* ifacep) {
        மாறிலி auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        எனில் (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](மாறிலி AstVar* varp) { திரும்பு varp->isParam(); },
                pair.first->second);
        }
        திரும்பு pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_முனை* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* மாறிலி lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* மாறிலி rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        எனில் (!lhsp || !rhsp) திரும்பு nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        எனில் (VL_UNCOVERABLE(!m_modp)) திரும்பு nullptr;

        மாறிலி auto& refVars = modIfaceRefs();
        மாறிலி auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        எனில் (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) திரும்பு nullptr;
        AstVar* மாறிலி ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* மாறிலி ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        எனில் (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) திரும்பு nullptr;

        மாறிலி auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        மாறிலி auto targetVarIt = ifaceVars.find(rhsp->name());
        எனில் (targetVarIt == ifaceVars.end()) திரும்பு nullptr;
        AstVar* மாறிலி targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        எனில் (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        எனில் (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            திரும்பு nullptr;  // LCOV_EXCL_LINE
        }
        திரும்பு targetVarp->valuep()->clone_மரம்(பொய்);
    }
    வெறுமை resolveGenericIfaceDotsIn(Ast_முனை* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.தள்ளு_back(dotp); });
        க்கு (AstDot* மாறிலி dotp : dotps) {
            எனில் (Ast_முனை* மாறிலி newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_மரம்(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    வெறுமை visit(AstVar* nodep) override {
        எனில் (nodep->user2SetOnce()) திரும்பு;  // Process once
        // Interface array port: split it into its elements
        எனில் (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(உண்மை);
        }
        // Build cache of interface port names as we encounter them
        எனில் (nodep->isIfaceRef()) { m_ifacePortNames.செருகு(nodep->name()); }
        iterateChildren(nodep);
        எனில் (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            எனில் (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } இல்லையெனில் எனில் (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                தருக்க hasUnresolvedLparamXRef = பொய்;
                nodep->valuep()->foreach([&](மாறிலி AstVarXRef* xrefp) {
                    எனில் (மாறிலி AstVar* மாறிலி varp = xrefp->varp()) {
                        எனில் (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = உண்மை;
                        }
                    }
                });
                எனில் (hasUnresolvedLparamXRef) திரும்பு;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                மாறிலி தருக்க hasDot = nodep->valuep()->exists([](AstDot*) { திரும்பு உண்மை; });
                தருக்க refsDeferred = பொய்;
                எனில் (!hasDot) {
                    மாறிலி auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](மாறிலி AstVarRef* refp) {
                        எனில் (refsDeferred) திரும்பு;
                        AstVar* மாறிலி refVarp = refp->varp();
                        எனில் (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.மொத்தம்(refVarp)) {
                            refsDeferred = உண்மை;
                        }
                    });
                }
                எனில் (hasDot || refsDeferred) {
                    v3Global.rootp()->தள்ளு_Deferred_Param_Varp(nodep);
                    திரும்பு;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        எனில் (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    வெறுமை visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        எனில் (nodep->varp()) iterate(nodep->varp());
    }
    தருக்க ifaceParamReplace(AstVarXRef* nodep, Ast_முனை* candp) {
        க்கு (; candp; candp = candp->nextp()) {
            எனில் (nodep->name() == candp->name()) {
                எனில் (AstVar* மாறிலி varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    எனில் (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    திரும்பு உண்மை;
                } இல்லையெனில் எனில் (மாறிலி AstPin* மாறிலி pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_மரம்(பொய்));
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு உண்மை;
                }
            }
        }
        திரும்பு பொய்;
    }

    வெறுமை deparamIfaceCellNow(AstCell* cellp) {
        எனில் (!cellp->paramsp()) திரும்பு;
        எனில் (!VN_IS(cellp->modp(), Iface)) திரும்பு;
        Ast_முனை_Module* மாறிலி srcModp = cellp->modp();
        Ast_முனை_Module* மாறிலி newModp
            = m_processor.முனை_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        எனில் (newModp && newModp != srcModp) {
            எனில் (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        எனில் (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        எனில் (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarXRef* nodep) override {
        எனில் (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            திரும்பு;
        }
        // Parameters of an interface array are accessed through its first element
        எனில் (nodep->varp() && nodep->varp()->isParam()) {
            மாறிலி auto it = m_ifaceInstCells.find(nodep->dotted());
            எனில் (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        மாறிலி string dotted = nodep->dotted();
        எனில் (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            மாறிலி Ast_முனை* backp = nodep;
            போது ((backp = backp->backp())) {
                எனில் (VN_IS(backp, முனை_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    நிறுத்து;
                }
                எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(backp, Var)) {
                    எனில் (!varp->isIfaceRef()) தொடர்;
                    // Through all dimensions of an array
                    மாறிலி AstIfaceRefDType* மாறிலி ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    எனில் (!ifacerefp) தொடர்;
                    // Interfaces passed in on the port map have ifaces
                    எனில் (மாறிலி AstIface* மாறிலி ifacep = ifacerefp->ifacep()) {
                        எனில் (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            எனில் (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                திரும்பு;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    இல்லையெனில் எனில் (AstCell* மாறிலி cellp = ifacerefp->cellp()) {
                        எனில் (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            எனில் (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                திரும்பு;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            மாறிலி auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            எனில் (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* மாறிலி cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    வெறுமை visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        மாறிலி AstClass* lhsClassp = nullptr;
        மாறிலி AstClassOrPackageRef* மாறிலி classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        எனில் (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_முனை* rhsDefp = nullptr;
        AstClassOrPackageRef* மாறிலி rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        எனில் (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        எனில் (lhsClassp && rhsDefp) {
            m_state.m_dots.தள்ளு_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } இல்லையெனில் {
            iterate(nodep->rhsp());
        }
    }

    வெறுமை visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* மாறிலி varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_முனை_FTask_Ref* மாறிலி taskrefp = VN_CAST(nodep->refp(), முனை_FTask_Ref);
        AstEnumItemRef* மாறிலி enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        எனில் (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } இல்லையெனில் எனில் (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } இல்லையெனில் எனில் (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } இல்லையெனில் {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            திரும்பு;
        }
        iterate(nodep->cellrefp());

        எனில் (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } இல்லையெனில் எனில் (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } இல்லையெனில் {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        க்கு (Ast_முனை *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<முழு> indices;
        க்கு (Ast_முனை* s = nodep->selp(); s; s = s->nextp()) {
            மாறிலி AstConst* மாறிலி constp = VN_CAST(s, Const);
            எனில் (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                திரும்பு;
            }
            indices.தள்ளு_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        மாறிலி string viftopSuffix = "__Viftop";
        மாறிலி string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        மாறிலி string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        எனில் (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            திரும்பு;
        }
        pos += baseName.length();
        க்கு (முழு idx : indices) {
            மாறிலி string replacement = "__BRA__" + Ast_முனை::encode_எண்(idx) + "__KET__";
            எனில் (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                திரும்பு;
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
    வெறுமை iterateGenerateCond(Ast_முனை* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = உண்மை;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    வெறுமை visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->condp(), Const)) {
            எனில் (Ast_முனை* மாறிலி keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } இல்லையெனில் {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            // Normal edit rules will now recurse the replacement
        } இல்லையெனில் {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    வெறுமை visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        எனில் (AstGenFor* மாறிலி forp = VN_AS(nodep->genforp(), GenFor)) {
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
            மாறிலி string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            எனில் (Ast_முனை* மாறிலி stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } இல்லையெனில் {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    வெறுமை visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        தருக்க hit = பொய்;
        Ast_முனை* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        க்கு (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            க்கு (Ast_முனை* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        மாறிலி AstConst* மாறிலி exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        க்கு (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            க்கு (Ast_முனை* ep = itemp->condsp(); ep;) {
                Ast_முனை* மாறிலி nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        க்கு (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            எனில் (!itemp->isDefault()) {
                க்கு (Ast_முனை* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    எனில் (மாறிலி AstConst* மாறிலி ccondp = VN_CAST(ep, Const)) {
                        V3_எண் match{nodep, 1};
                        match.opEq(ccondp->இலக்கம்(), exprp->இலக்கம்());
                        எனில் (!hit && match.isNeqZero()) {
                            hit = உண்மை;
                            keepp = itemp->itemsp();
                        }
                    } இல்லையெனில் {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        க்கு (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            எனில் (itemp->isDefault()) {
                எனில் (!hit) {
                    hit = உண்மை;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        எனில் (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = இயல்பு;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

வகுப்பு Param_உச்சி final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    வெறுமை relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        க்கு (AstDot* மாறிலி dotp : m_state.m_dots) {
            மாறிலி AstClassOrPackageRef* மாறிலி classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            மாறிலி AstClass* மாறிலி lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* மாறிலி rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            க்கு (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                எனில் (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    நிறுத்து;
                }
            }
        }
    }

    வெறுமை fixLevel(Ast_முனை_Module* modp) {
        // Fix up level of module, based on who instantiates it
        எனில் (modp->user1SetOnce()) திரும்பு;  // Already fixed
        எனில் (m_state.m_parentps[modp].empty()) திரும்பு;  // Leave top levels alone
        முழு maxParentLevel = 0;
        க்கு (Ast_முனை_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        எனில் (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    வெறுமை resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_முனை_Module*> modps;
        க்கு (Ast_முனை_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), முனை_Module);
            modp->unlinkFrBack();
            modps.தள்ளு_back(modp);
        }

        // Fix-up levels
        மாறிலி VNUser1InUse user1InUse;
        க்கு (Ast_முனை_Module* மாறிலி modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](மாறிலி Ast_முனை_Module* ap, மாறிலி Ast_முனை_Module* bp) {
                             திரும்பு ap->level() < bp->level();
                         });

        // Re-insert modules
        க்கு (Ast_முனை_Module* மாறிலி modp : modps) netlistp->addModulesp(modp);
    }

பொது:
    // CONSTRUCTORS
    explicit Param_உச்சி(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        மாறிலி ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        மாறிலி ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_முனை_FTask_Ref* ftaskrefp) {
            Ast_முனை_FTask* ftaskp = ftaskrefp->taskp();
            எனில் (!ftaskp || !ftaskp->classMethod()) திரும்பு;
            மாறிலி std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            மாறிலி auto ancestorClassOf = [](Ast_முனை* startp) -> AstClass* {
                க்கு (Ast_முனை* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    எனில் (AstClass* மாறிலி cp = VN_CAST(np, Class)) திரும்பு cp;
                }
                திரும்பு nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            எனில் (refClassp == ftaskrefp->classOrPackagep())
                திரும்பு;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            எனில் (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                திரும்பு;
            }
            எனில் (classp->user3p()) திரும்பு;  // will not get removed, no need to relink
            AstClass* மாறிலி parametrizedClassp = VN_CAST(classp->user4p(), Class);
            எனில் (!parametrizedClassp) திரும்பு;
            Ast_முனை_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_முனை_FTask* ftaskp) {
                எனில் (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    திரும்பு உண்மை;
                }
                திரும்பு பொய்;
            });
            எனில் (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        மாறிலி ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        க்கு (AstClass* மாறிலி classp : m_state.m_paramClasses) {
            எனில் (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                எனில் (V3Error::error_மொத்தம்()) தொடர்;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(தள்ளு_Deletep(classp->unlinkFrBack()), classp);
            } இல்லையெனில் {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(பொய்);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_முனை::user3_Clear_மரம்();
        க்கு (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_முனை* மாறிலி nodep) { nodep->user3(உண்மை); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_முனை* மாறிலி nodep) {
            nodep->foreachLink([&](Ast_முனை** மாறிலி linkpp, மாறிலி எழுத்து* namep) {
                எனில் (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_உச்சி() = இயல்பு;
    VL_UNCOPYABLE(Param_உச்சி);
};

//######################################################################
// Param class functions

வெறுமை V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    எனில் (dump_மரம்_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_உச்சி{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    எனில் (dump_மரம்_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_மரம்("param", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    க்கு (AstVar* மாறிலி varp : rootp->deferredParamVarps()) {
        எனில் (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
