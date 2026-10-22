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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Param.h"

#ଅନ୍ତର୍ଭାଵ "V3Case.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3HierBlock.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkDotIfaceCapture.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3Simulate.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3Unroll.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"

#ଅନ୍ତର୍ଭାଵ <cctype>
#ଅନ୍ତର୍ଭାଵ <cstdlib>
#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ସ୍ଥିର AstClassRefDType* class_Ref_DType_Of_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
    ଯେତେବେଳେ (AstTypedef* ଅଚଳ tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(nodep, ଗଣ୍ଠି_DType);
    ଫେରନ୍ତୁ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

ଶ୍ରେଣୀ ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ଅଚଳ V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ଅଚଳ std::string, Ast_ଗଣ୍ଠି_Module*>;
    using ParamConstMap = std::map<ଅଚଳ std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ଅଚଳ std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ଅଚଳ ତାର୍କିକ m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<ଅଚଳ V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ଅଚଳ std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<ଅଚଳ AstVar*> m_untypedParams;

    // METHODS

ସର୍ବସାଧାରଣ:
    ParameterizedHierBlocks(ଅଚଳ V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        ପାଇଁ (ଅଚଳ auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ଅଚଳ V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            ପାଇଁ (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ଅଚଳ ତାର୍କିକ inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ଭର୍ତ୍ତି({hierOpt.second.origName(), {}});
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
            ଯଦି (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ଅଚଳ string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ଅଚଳ auto defParamIt = m_modParams.find(actualModName);
            ଯଦି (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                        ଯଦି (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ଅଚଳ AstBasicDType* ଅଚଳ dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        ଯଦି (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    ତାର୍କିକ hierSubRun() ଅଚଳ { ଫେରନ୍ତୁ m_hierSubRun; }
    ତାର୍କିକ isHierBlock(ଅଚଳ string& origName) ଅଚଳ {
        ଫେରନ୍ତୁ m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_ଗଣ୍ଠି_Module* findByParams(ଅଚଳ string& origName, AstPin* firstPinp,
                                ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ଅଚଳ std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        ଅଚଳ auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ଅଚଳ AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        ପାଇଁ (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            ତାର୍କିକ found = ସତ୍ୟ;
            size_t paramIdx = 0;
            ଅଚଳ ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            ପାଇଁ (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (!pinp->exprp()) ଜାରି;
                ଯଦି (ଅଚଳ AstVar* ଅଚଳ modvarp = pinp->modVarp()) {
                    AstConst* ଅଚଳ constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ଅଚଳ auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ଅଚଳ ତାର୍କିକ untyped = m_untypedParams.ଗଣନା_2(paramIt->second);
                    AstConst* ଅଚଳ defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    ଯଦି (defValuep && areSame(constp, defValuep)) {
                        ଯଦି (!untyped || constp->same_ମୂଲ୍ୟ_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ଜାରି;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ଅଚଳ auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    ଯଦି (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = ମିଥ୍ୟା;
                        ଭାଙ୍ଗ;
                    }
                    ଯଦି (untyped && !constp->same_ମୂଲ୍ୟ_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = ମିଥ୍ୟା;
                        ଭାଙ୍ଗ;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            ଯଦି (found && paramIdx == hierIt->second->params().size()) ଭାଙ୍ଗ;
        }
        ଯଦି (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ଗଣ୍ଠି::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            ଫେରନ୍ତୁ nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ଅଚଳ HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ଅଚଳ auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        ଯଦି (it == m_hierBlockMod.end()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ it->second;
    }
    ସ୍ଥିର ତାର୍କିକ areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        ଯଦି (pinValuep->isString()) {
            ଫେରନ୍ତୁ pinValuep->ଅଙ୍କ().toString() == hierOptParamp->ଅଙ୍କ().toString();
        }

        ଯଦି (hierOptParamp->isDouble()) {
            ଦ୍ୱିଗୁଣ var;
            ଯଦି (pinValuep->isDouble()) {
                var = pinValuep->ଅଙ୍କ().toDouble();
            } ଅନ୍ୟଥା {  // Cast from integer to real
                V3_ସଂଖ୍ୟା var_ଅଙ୍କ{pinValuep, V3_ସଂଖ୍ୟା::Double{}, 0.0};
                var_ଅଙ୍କ.opIToRD(pinValuep->ଅଙ୍କ());
                var = var_ଅଙ୍କ.toDouble();
            }
            ଫେରନ୍ତୁ V3_ସଂଖ୍ୟା::epsilonEqual(var, hierOptParamp->ଅଙ୍କ().toDouble());
        } ଅନ୍ୟଥା {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_ସଂଖ୍ୟା var_ଅଙ୍କ{pinValuep, hierOptParamp->ଅଙ୍କ().width()};
            ଯଦି (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_ଅଙ୍କ.opRToIRoundS(pinValuep->ଅଙ୍କ());
            } ଅନ୍ୟଥା ଯଦି (pinValuep->isSigned()) {
                var_ଅଙ୍କ.opExtendS(pinValuep->ଅଙ୍କ(), pinValuep->ଅଙ୍କ().width());
            } ଅନ୍ୟଥା {
                var_ଅଙ୍କ.opAssign(pinValuep->ଅଙ୍କ());
            }
            V3_ସଂଖ୍ୟା isEq{pinValuep, 1};
            isEq.opEq(var_ଅଙ୍କ, hierOptParamp->ଅଙ୍କ());
            ଫେରନ୍ତୁ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

ଶ୍ରେଣୀ ParamProcessor final {
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
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;
    ଅଚଳ VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, Ast_ଗଣ୍ଠି*>;
    ସଂରଚନା ModInfo final {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_ଗଣ୍ଠି_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ଅଚଳ std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ଅଚଳ std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ଅଚଳ std::string, std::string> m_ମୂଲ୍ୟ_Names;  // Parameter value text to its name
    std::set<std::string> m_ମୂଲ୍ୟ_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ସ୍ଥିର constexpr std::string::size_type DIGEST_DIGITS = 8;

    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_ମୂଲ୍ୟ_Map = std::map<std::string, Ast_ଗଣ୍ଠି*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ଗଣ୍ଠି_Module*, Default_ମୂଲ୍ୟ_Map> m_default_Parameter_ମୂଲ୍ୟଗୁଡ଼ିକ;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ଅଚଳ Ast_ଗଣ୍ଠି_Module*, ଅଚଳ AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ଅଚଳ AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ସ୍ଥିର ଶୂନ୍ୟ makeSmallNames(Ast_ଗଣ୍ଠି_Module* modp) {
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isGParam() || varp->isIfaceRef()) {
                    ଅକ୍ଷର ch = varp->name()[0];
                    ch = std::toupper(ch);
                    ଯଦି (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(ch)] * 256 + ch);
                    usedLetter[static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(ch)]++;
                }
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ typep = VN_CAST(stmtp, ParamTypeDType)) {
                ଅଚଳ ଅକ୍ଷର ch = 'T';
                typep->user3(usedLetter[static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(ch)] * 256 + ch);
                usedLetter[static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(ch)]++;
            }
        }
    }
    ସ୍ଥିର string paramSmallName(Ast_ଗଣ୍ଠି_Module* modp, Ast_ଗଣ୍ଠି* varp) {
        ଯଦି (varp->user3() <= 1) makeSmallNames(modp);
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା index = varp->user3() / 256;
        ଅଚଳ ଅକ୍ଷର ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        ଯେତେବେଳେ (index) {
            st += cvtToStr(static_cast<ଅକ୍ଷର>((index % 25) + 'A'));
            index /= 26;
        }
        ଫେରନ୍ତୁ st;
    }

    ସ୍ଥିର ତାର୍କିକ is_Aggregate_Param_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ସ୍ଥିର string param_ମୂଲ୍ୟ_String(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (ଅଚଳ AstRefDType* ଅଚଳ refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string ଚାବି = nodep->name();
        ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            ଯଦି (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ଚାବି = ifrtp->cellp()->modp()->name();
            } ଅନ୍ୟଥା ଯଦି (ifrtp->ifacep()) {
                ଚାବି = ifrtp->ifacep()->name();
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ dtypep
                   = VN_CAST(nodep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଚାବି += " ";
            ଚାବି += dtypep->verilogKwd();
            ଚାବି += " {";
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ଚାବି += param_ମୂଲ୍ୟ_String(memberp);
                ଚାବି += ";";
            }
            ଚାବି += "}";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberDType* ଅଚଳ dtypep = VN_CAST(nodep, MemberDType)) {
            ଚାବି += " ";
            ଚାବି += param_ମୂଲ୍ୟ_String(dtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ dtypep = VN_CAST(nodep, BasicDType)) {
            ଯଦି (dtypep->isSigned()) ଚାବି += " signed";
            ଯଦି (dtypep->isRanged()) {
                ଚାବି += "[" + cvtToStr(dtypep->ବାମ()) + ":" + cvtToStr(dtypep->ଡାହାଣ()) + "]";
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ dtypep = VN_CAST(nodep, PackArrayDType)) {
            ଚାବି += "[";
            ଚାବି += cvtToStr(dtypep->ବାମ());
            ଚାବି += ":";
            ଚାବି += cvtToStr(dtypep->ଡାହାଣ());
            ଚାବି += "] ";
            ଚାବି += param_ମୂଲ୍ୟ_String(dtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPattern* ଅଚଳ patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            ଚାବି += "'{";
            ପାଇଁ (ଅଚଳ AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                ଯଦି (memp->isDefault()) ଚାବି += "default";
                ଯଦି (ଅଚଳ AstText* ଅଚଳ textp = VN_CAST(memp->keyp(), Text)) {
                    ଚାବି += textp->text();
                } ଅନ୍ୟଥା ଯଦି (memp->keyp()) {
                    ଚାବି += param_ମୂଲ୍ୟ_String(memp->keyp());
                }
                ଚାବି += ":";
                ଯଦି (memp->repp()) ଚାବି += param_ମୂଲ୍ୟ_String(memp->repp()) + "x";
                ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                    ଚାବି += param_ମୂଲ୍ୟ_String(valuep) + ";";
                }
                ଚାବି += ",";
            }
            ଚାବି += "}";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ଚାବି += "{";
            ପାଇଁ (ଅଚଳ AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                ଚାବି += cvtToStr(itemp->index()) + ":" + param_ମୂଲ୍ୟ_String(itemp->valuep()) + ",";
            }
            ଯଦି (consArrayp->defaultp()) {
                ଚାବି += "default:" + param_ମୂଲ୍ୟ_String(consArrayp->defaultp()) + ",";
            }
            ଚାବି += "}";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConsStruct* ଅଚଳ consp = VN_CAST(nodep, ConsStruct)) {
            ଚାବି += "{";
            ପାଇଁ (ଅଚଳ AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                ଚାବି += param_ମୂଲ୍ୟ_String(itemp->valuep());
                ଚାବି += ",";
            }
            ଚାବି += "}";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            ଯଦି (classRefp->classp()) {
                ଅଚଳ string& className = classRefp->classp()->name();
                ଅଚଳ string& origName = classRefp->classp()->origName();
                ଅଚଳ ତାର୍କିକ isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                ଯଦି (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } ଅନ୍ୟଥା ଯଦି (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ଚାବି = className;
                } ଅନ୍ୟଥା {
                    // Unspecialized class with no params - use origName
                    ଚାବି = origName;
                }
            } ଅନ୍ୟଥା {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ଚାବି = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ dtypep = VN_CAST(nodep, QueueDType)) {
            ଚାବି = paramElemString(dtypep->subDTypep()) + "$[$";
            ଯଦି (dtypep->boundConst()) ଚାବି += ":" + cvtToStr(dtypep->boundConst());
            ଚାବି += "]";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ dtypep = VN_CAST(nodep, DynArrayDType)) {
            ଚାବି = paramElemString(dtypep->subDTypep()) + "$[]";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAssocArrayDType* ଅଚଳ dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ଚାବି = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ଚାବି_DTypep()) + "]";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstWildcardArrayDType* ଅଚଳ dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ଚାବି = paramElemString(dtypep->subDTypep()) + "$[*]";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstEnumDType* ଅଚଳ dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ଚାବି += " enum {";
            ପାଇଁ (ଅଚଳ AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(itemp->valuep(), Const);
                ଚାବି += itemp->name() + "=" + (constp ? constp->ଅଙ୍କ().ascii() : "?") + ";";
            }
            ଚାବି += "}";
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = VN_CAST(nodep, ଗଣ୍ଠି_DType)) {
            ଚାବି += dtypep->prettyDTypeName(ସତ୍ୟ);
        }
        UASSERT_OBJ(!ଚାବି.empty(), nodep, "Parameter yielded no value string");
        ଫେରନ୍ତୁ ଚାବି;
    }
    // As paramValueString(), for the element or key type of a container
    ସ୍ଥିର string paramElemString(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ଅଚଳ AstClassRefDType* ଅଚଳ classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        ଯଦି (classRefp && classRefp->paramsp()) ଫେରନ୍ତୁ classRefp->prettyDTypeName(ସତ୍ୟ);
        ଫେରନ୍ତୁ param_ମୂଲ୍ୟ_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ସ୍ଥିର string digestSuffix(ଅଚଳ string& text, std::set<string>& usedr) {
        ଅଚଳ string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        ଯେତେବେଳେ (digits < hex.size() && !usedr.ଭର୍ତ୍ତି(hex.substr(0, digits)).second) ++digits;
        ଫେରନ୍ତୁ hex.substr(0, digits);
    }
    string param_ମୂଲ୍ୟ_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        ଯଦି (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ଅଚଳ string text = param_ମୂଲ୍ୟ_String(nodep);
        ଅଚଳ auto pair = m_ମୂଲ୍ୟ_Names.emplace(text, "");
        ଯଦି (pair.second) pair.first->second = "z" + digestSuffix(text, m_ମୂଲ୍ୟ_Suffixes);
        ଫେରନ୍ତୁ pair.first->second;
    }
    string moduleCalcName(ଅଚଳ Ast_ଗଣ୍ଠି_Module* srcModp, ଅଚଳ string& longname) {
        string newname = longname;
        ଯଦି (longname.length() > 30) {
            ଅଚଳ auto pair = m_longMap.emplace(longname, "");
            ଯଦି (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        ଫେରନ୍ତୁ newname;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* arraySubDTypep(Ast_ଗଣ୍ଠି_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            ଫେରନ୍ତୁ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        ଯଦି (ଅଚଳ AstBracketArrayDType* ଅଚଳ adtypep = VN_CAST(nodep, BracketArrayDType)) {
            ଫେରନ୍ତୁ adtypep->subDTypep();
        }
        ଫେରନ୍ତୁ nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* arraySubDTypeDeepp(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯେତେବେଳେ (Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = arraySubDTypep(nodep)) nodep = subp;
        ଫେରନ୍ତୁ nodep;
    }
    ସ୍ଥିର ତାର୍କିକ isString(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯଦି (AstBasicDType* ଅଚଳ basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            ଫେରନ୍ତୁ basicp->isString();
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ collectPins(CloneMap* clonemapp, Ast_ଗଣ୍ଠି_Module* modp, ତାର୍କିକ originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଅଚଳ Ast_ଗଣ୍ଠି* originalParamp = nullptr;
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ଯଦି (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ଯଦି (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            ଯଦି (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    ଶୂନ୍ୟ relinkPins(ଅଚଳ CloneMap* clonemapp, AstPin* startpinp) {
        ପାଇଁ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ଅଚଳ auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } ଅନ୍ୟଥା ଯଦି (pinp->modPTypep()) {
                ଅଚଳ auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } ଅନ୍ୟଥା {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    ଶୂନ୍ୟ relinkPinsByName(AstPin* startpinp, Ast_ଗଣ୍ଠି_Module* modp) {
        std::map<ଅଚଳ string, AstVar*> nameToPin;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        ପାଇଁ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = pinp->modVarp()) {
                ଅଚଳ auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    ସ୍ଥିର ତାର୍କିକ hasDescendantDefparams(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                       std::set<ଅଚଳ Ast_ଗଣ୍ଠି_Module*>& visited) {
        ଯଦି (!visited.ଭର୍ତ୍ତି(modp).second) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ଅଚଳ cellp = VN_CAST(stmtp, Cell);
            ଯଦି (!cellp) ଜାରି;
            ପାଇଁ (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
            ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ childModp = cellp->modp()) {
                ଯଦି (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }
        visited.erase(modp);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର ତାର୍କିକ hasDescendantDefparams(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        std::set<ଅଚଳ Ast_ଗଣ୍ଠି_Module*> visited;
        ଫେରନ୍ତୁ hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ଅଚଳ AstPin* nestedDefparamp(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଅଚଳ auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        ଯଦି (!pair.second) ଫେରନ୍ତୁ pair.first->second;  // Searched, or being searched
        ଅଚଳ AstPin* foundp = nullptr;
        modp->exists([&](ଅଚଳ AstCell* cellp) {
            ପାଇଁ (ଅଚଳ AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (!pinp->paramPath().empty()) foundp = pinp;
            }
            ଯଦି (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            ଫେରନ୍ତୁ foundp != nullptr;
        });
        pair.first->second = foundp;
        ଫେରନ୍ତୁ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    ଶୂନ୍ୟ checkSupportedParam(Ast_ଗଣ୍ଠି_Module* modp, AstPin* pinp) ଅଚଳ {
        // ConsArray is not supported because that can not be set via -G
        // option.
        ଯଦି (pinp->modVarp()) {
            ତାର୍କିକ supported = ମିଥ୍ୟା;
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(pinp->exprp(), Const)) {
                supported = ସତ୍ୟ;
                ଯଦି (constp->ଅଙ୍କ().isString()
                    && !V3HierBlock::stringParamPassable(constp->ଅଙ୍କ().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            ଯଦି (!supported) {
                pinp->v3error(
                    Ast_ଗଣ୍ଠି::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstParamTypeDType* ଅଚଳ typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ଗଣ୍ଠି::prettyNameQ(modp->origName()));
        }
    }
    ତାର୍କିକ moduleExists(ଅଚଳ string& modName) ଅଚଳ {
        ଯଦି (m_allModuleNames.find(modName) != m_allModuleNames.end()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_modNameMap.find(modName) != m_modNameMap.end()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    string parameterizedHierBlockName(Ast_ଗଣ୍ଠି_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ଗଣ୍ଠି*> pins;

        AstPin* pinp = paramPinsp;
        ଯେତେବେଳେ (pinp) {
            checkSupportedParam(modp, pinp);
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = pinp->modVarp()) {
                ଯଦି (!pinp->exprp()) ଜାରି;
                ଯଦି (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } ଅନ୍ୟଥା ଯଦି (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ଗଣ୍ଠି_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ଅଚଳ auto pair = m_default_Parameter_ମୂଲ୍ୟଗୁଡ଼ିକ.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        ଯଦି (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            ଯଦି (ଅଚଳ AstPin* ଅଚଳ defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ଗଣ୍ଠି::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_ମୂଲ୍ୟ_Map params;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                    ଯଦି (varp->isGParam()) {
                        AstConst* ଅଚଳ constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ dtypep = static_cast<Ast_ଗଣ୍ଠି*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ଅଚଳ auto paramsIt = pair.first;
        ଯଦି (paramsIt->second.empty()) ଫେରନ୍ତୁ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        ପାଇଁ (auto&& default_ମୂଲ୍ୟ : paramsIt->second) {
            ଅଚଳ auto pinIt = pins.find(default_ମୂଲ୍ୟ.first);
            // If the pin does not have a value assigned, use the default one.
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep = pinIt == pins.end() ? default_ମୂଲ୍ୟ.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_ମୂଲ୍ୟ.first + "=";
            // constp can be nullptr

            ଯଦି (ଅଚଳ AstConst* ଅଚଳ p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->ଅଙ୍କ().ascii(ମିଥ୍ୟା);
            } ଅନ୍ୟଥା ଯଦି (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ବୃକ୍ଷ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ଅଚଳ auto iter = m_longMap.find(longname);
        ଯଦି (iter != m_longMap.end()) ଫେରନ୍ତୁ iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ଭର୍ତ୍ତି(longname);
        ଯେତେବେଳେ (ସତ୍ୟ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ଅଚଳ string hashStr = hashStrGen.digestHex();
            ପାଇଁ (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                ଯଦି (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                ଯଦି (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    ଫେରନ୍ତୁ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ଭର୍ତ୍ତି(V3Os::trueRandom(64));
        }
    }
    ଶୂନ୍ୟ replaceRefsRecurse(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, ଅଚଳ AstClass* ଅଚଳ oldClassp,
                            AstClass* ଅଚଳ newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        ଯଦି (AstClassRefDType* ଅଚଳ classRefp = VN_CAST(nodep, ClassRefDType)) {
            ଯଦି (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } ଅନ୍ୟଥା ଯଦି (AstClassOrPackageRef* ଅଚଳ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ଯଦି (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } ଅନ୍ୟଥା ଯଦି (AstTypedef* ଅଚଳ typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            ଯଦି (typedefp->subDTypep()) {
                ଯଦି (AstClassRefDType* ଅଚଳ classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    ଯଦି (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskRefp = VN_CAST(nodep, ଗଣ୍ଠି_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            ଯଦି (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ oldTaskp = ftaskRefp->taskp()) {
                ଯଦି (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        ଯଦି (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        ଯଦି (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        ଯଦି (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        ଯଦି (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        ଯଦି (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    ଶ୍ରେଣୀ VarXRefRelinkVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
        Ast_ଗଣ୍ଠି_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ଗଣ୍ଠି_Module*> m_varModuleMap;  // Cache var->module lookups

    ସର୍ବସାଧାରଣ:
        explicit VarXRefRelinkVisitor(Ast_ଗଣ୍ଠି_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ବ୍ୟକ୍ତିଗତ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ଗଣ୍ଠି_Module* findVarModule(AstVar* varp) {
            ଅଚଳ auto it = m_varModuleMap.find(varp);
            ଯଦି (it != m_varModuleMap.end()) ଫେରନ୍ତୁ it->second;
            Ast_ଗଣ୍ଠି_Module* varModp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି* np = varp; np; np = np->backp()) {
                ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(np, ଗଣ୍ଠି_Module)) {
                    varModp = modp;
                    ଭାଙ୍ଗ;
                }
            }
            m_varModuleMap[varp] = varModp;
            ଫେରନ୍ତୁ varModp;
        }

        ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
            AstVar* ଅଚଳ varp = nodep->varp();
            ଯଦି (!varp) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ଅଚଳ string& dotted = nodep->dotted();
            ଯଦି (dotted.empty()) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            ଅଚଳ size_t dotPos = dotted.find('.');
            ଅଚଳ string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            ଯଦି (portName.empty()) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (AstVar* ଅଚଳ varChkp = VN_CAST(stmtp, Var)) {
                    ଯଦି (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        ଭାଙ୍ଗ;
                    }
                }
            }
            ଯଦି (!portVarp) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ଅଚଳ irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            ଯଦି (!irefp) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            Ast_ଗଣ୍ଠି_Module* ଅଚଳ newIfacep = irefp->ifaceViaCellp();
            ଯଦି (!newIfacep) {
                iterateChildren(nodep);
                ଫେରନ୍ତୁ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            ଯଦି (varModp && varModp != newIfacep) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (AstVar* ଅଚଳ newVarp = VN_CAST(stmtp, Var)) {
                        ଯଦି (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            ଭାଙ୍ଗ;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    ଶୂନ୍ୟ fixupCrossModuleRefDTypes(Ast_ଗଣ୍ଠି_Module* newModp) {
        ଯଦି (!V3LinkDotIfaceCapture::enabled()) ଫେରନ୍ତୁ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା fixed_ଗଣନା = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            ଯଦି (!tagp) ଫେରନ୍ତୁ;

            Ast_ଗଣ୍ଠି_Module* ଅଚଳ correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            ଯଦି (!correctModp || correctModp->dead()) ଫେରନ୍ତୁ;
            ଯଦି (correctModp->parameterizedTemplate()) ଫେରନ୍ତୁ;

            ତାର୍କିକ fixed = ମିଥ୍ୟା;
            ଯଦି (refp->typedefp()) {
                ଯଦି (AstTypedef* ଅଚଳ newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    ଯଦି (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = ସତ୍ୟ;
                }
            }
            ଯଦି (!fixed && refp->refDTypep()) {
                ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = ସତ୍ୟ;
                }
            }
            ଯଦି (fixed) ++fixed_ଗଣନା;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_ଗଣନା);
    }

    // Return true on success, false on error
    ତାର୍କିକ deepCloneModule(Ast_ଗଣ୍ଠି_Module* srcModp, Ast_ଗଣ୍ଠି* ifErrorp, AstPin* paramsp,
                         ଅଚଳ string& newname, ଅଚଳ IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ଗଣ୍ଠି_Module* newModp;
        ଯଦି (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ଗଣ୍ଠି_Module);
        } ଅନ୍ୟଥା {
            newModp = srcModp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        ଯଦି (AstIface* ଅଚଳ newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(ସତ୍ୟ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(ମିଥ୍ୟା);

        newModp->name(newname);
        newModp->user2(ମିଥ୍ୟା);  // We need to re-recurse this module once changed
        newModp->recursive(ମିଥ୍ୟା);
        newModp->recursiveClone(ମିଥ୍ୟା);
        newModp->hasGParam(ମିଥ୍ୟା);
        // Recursion may need level cleanups
        ଯଦି (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        ଯଦି ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_ବୃକ୍ଷ(), newModp);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        ଯଦି (AstClass* ଅଚଳ newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ଗଣ୍ଠି* insertp = srcModp;
        ଯେତେବେଳେ (VN_IS(insertp->nextp(), ଗଣ୍ଠି_Module)
               && VN_AS(insertp->nextp(), ଗଣ୍ଠି_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ଅଚଳ auto iter = m_modNameMap.find(newname);
        CloneMap* ଅଚଳ clonemapp = &(iter->second.m_cloneMap);
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
        ତାର୍କିକ needRelinkByName = ମିଥ୍ୟା;
        ଯଦି (paramsp) {
            ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
            }
        }
        ଯଦି (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } ଅନ୍ୟଥା {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        ପାଇଁ (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ଅଚଳ AstIfaceRefDType* ଅଚଳ portIrefp = it->first;
            ଅଚଳ AstIfaceRefDType* ଅଚଳ pinIrefp = it->second;
            AstIfaceRefDType* ଅଚଳ cloneIrefp = portIrefp->clonep();
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
        ଯଦି (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (pinp->exprp()) {
                ଯଦି (AstVar* ଅଚଳ modvarp = pinp->modVarp()) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ଅଚଳ exprp = VN_CAST(newp, Const);
                    AstConst* ଅଚଳ origp = VN_CAST(modvarp->valuep(), Const);
                    ଅଚଳ ତାର୍କିକ overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    ଯଦି (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    modvarp->overriddenParam(overridden);
                } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ modptp = pinp->modPTypep()) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = VN_AS(pinp->exprp(), ଗଣ୍ଠି_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    ଯଦି (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଅଚଳ ModInfo* moduleFindOrClone(Ast_ଗଣ୍ଠି_Module* srcModp, Ast_ଗଣ୍ଠି* ifErrorp, AstPin* paramsp,
                                     ଅଚଳ string& newname, ଅଚଳ IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        ଯଦି (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } ଅନ୍ୟଥା {
            ଯଦି (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                ଫେରନ୍ତୁ nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ଅଚଳ ModInfo* ଅଚଳ modInfop = &(it->second);
        ଫେରନ୍ତୁ modInfop;
    }

    ଶୂନ୍ୟ convertToStringp(Ast_ଗଣ୍ଠି* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ଅଚଳ constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        ଯଦି (constp && !constp->ଅଙ୍କ().isString()) {
            constp->replaceWith(
                ନୂତନ AstConst{constp->fileline(), AstConst::String{}, constp->ଅଙ୍କ().toString()});
            VL_DO_DANGLING(constp->delete_ବୃକ୍ଷ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    ଶୂନ୍ୟ constify_Member_ମୂଲ୍ୟ(AstVar* varp, std::set<AstVar*>& inProgress) {
        ଯଦି (!varp->valuep() || VN_IS(varp->valuep(), Const)) ଫେରନ୍ତୁ;
        ଯଦି (!inProgress.ଭର୍ତ୍ତି(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            varp->valuep(ନୂତନ AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            ଫେରନ୍ତୁ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ଅଚଳ AstVarRef* refp) {
            AstVar* ଅଚଳ refVarp = refp->varp();
            ଯଦି (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ଭର୍ତ୍ତି(refVarp);
            }
        });
        ପାଇଁ (AstVar* ଅଚଳ sibp : siblings) constify_Member_ମୂଲ୍ୟ(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ସ୍ଥିର AstParseRef* memberParseRef(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (AstParseRef* ଅଚଳ refp = VN_CAST(nodep, ParseRef)) ଫେରନ୍ତୁ refp;
        Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ selp = VN_CAST(nodep, ଗଣ୍ଠି_Pre_Sel);
        ଫେରନ୍ତୁ selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ସ୍ଥିର ଶୂନ୍ୟ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ selp = VN_AS(dotp->rhsp(), ଗଣ୍ଠି_Pre_Sel);
            ଯଦି (AstAttrOf* ଅଚଳ attrp = selp->attrp()) {
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    VL_DO_DANGLING(oldFromp->delete_ବୃକ୍ଷ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ବୃକ୍ଷ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_ବୃକ୍ଷ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ସ୍ଥିର ତାର୍କିକ isDeferredMemberBase(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (VN_IS(nodep, MemberSel)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ selp = VN_CAST(nodep, ଗଣ୍ଠି_Pre_Sel)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDTypep = selp->fromp()->dtypep();
            ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ଗଣ୍ଠି_Array_DType) : nullptr;
            ଯଦି (arrayDTypep) {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = arrayDTypep->subDTypep();
                ଫେରନ୍ତୁ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ଗଣ୍ଠି_UOr_Struct_DType);
            }
            ଫେରନ୍ତୁ isDeferredMemberBase(selp->fromp());
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->dtypep();
        ଫେରନ୍ତୁ dtypep && VN_IS(dtypep->skipRefOrNullp(), ଗଣ୍ଠି_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ସ୍ଥିର ଶୂନ୍ୟ resolveMemberDot(AstDot* dotp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = VN_CAST(dotp->lhsp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        AstParseRef* ଅଚଳ memberRefp = memberParseRef(dotp->rhsp());
        ଯଦି (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) ଫେରନ୍ତୁ;
        AstMemberSel* ଅଚଳ newp = ନୂତନ AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    ଶୂନ୍ୟ resolveDotToTypedef(Ast_ଗଣ୍ଠି* exprp) {
        AstDot* ଅଚଳ dotp = VN_CAST(exprp, Dot);
        ଯଦି (!dotp) ଫେରନ୍ତୁ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ଅଚଳ classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        ଯଦି (!classRefp) {
            resolveMemberDot(dotp);
            ଫେରନ୍ତୁ;
        }
        AstParseRef* ଅଚଳ parseRefp = memberParseRef(dotp->rhsp());
        ଯଦି (!parseRefp) ଫେରନ୍ତୁ;

        ଅଚଳ AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ଅଚଳ paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        ଯଦି (paramTypep) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = paramTypep->subDTypep();
            AstClassRefDType* ଅଚଳ classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            ଯଦି (classRefDTypep) {
                AstClass* ଅଚଳ srcClassp = classRefDTypep->classp();
                ଯଦି (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    ଯଦି (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        ଯଦି (classRefp->paramsp()) {
            AstClass* ଅଚଳ srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            ଯଦି (srcClassp && srcClassp->hasGParam()) {
                ଯଦି (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        ଯଦି (AstClassRefDType* ଅଚଳ crp = class_Ref_DType_Of_ଗଣ୍ଠି(classRefp->classOrPackageNodep())) {
            ଯଦି (AstClass* ଅଚଳ srcClassp = crp->classp()) {
                ଯଦି (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    ଯଦି (AstClass* ଅଚଳ newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        ଯଦି (!lhsClassp) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି* ଅଚଳ memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        ଯଦି (AstTypedef* ଅଚଳ tdefp = VN_CAST(memberp, Typedef)) {
            ଯଦି (parseRefp != dotp->rhsp()) ଫେରନ୍ତୁ;
            AstRefDType* ଅଚଳ refp = ନୂତନ AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ବୃକ୍ଷ(), dotp);
        } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    ଶୂନ୍ୟ substituteParamMember(AstDot* ଅଚଳ dotp, AstParseRef* ଅଚଳ memberRefp,
                               AstVar* ଅଚଳ varp) {
        ଯଦି (!varp->isParam() || !varp->valuep()) ଫେରନ୍ତୁ;
        ଯଦି (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_ମୂଲ୍ୟ(varp, inProgress);
        }
        AstConst* ଅଚଳ constp = VN_CAST(varp->valuep(), Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ;
        AstConst* ଅଚଳ newp = constp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    ଶୂନ୍ୟ resolveParamClassRefDType(AstRefDType* refp) {
        ଯଦି (refp->typedefp() || refp->refDTypep()) ଫେରନ୍ତୁ;

        AstClassOrPackageRef* ଅଚଳ classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        ଯଦି (!classRefp) ଫେରନ୍ତୁ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        ଯଦି (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            ଫେରନ୍ତୁ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ଗଣ୍ଠି* targetp = classRefp->classOrPackageNodep();
        ଯେତେବେଳେ (ଅଚଳ AstTypedef* ଅଚଳ tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtargetp = VN_CAST(targetp, ଗଣ୍ଠି_DType))
            targetp = dtargetp->skipRefOrNullp();
        ଯଦି (!targetp) ଫେରନ୍ତୁ;
        AstClassRefDType* ଅଚଳ classRefDTypep = VN_CAST(targetp, ClassRefDType);
        ଯଦି (!classRefDTypep) ଫେରନ୍ତୁ;
        srcClassp = classRefDTypep->classp();
        ଯଦି (!srcClassp) ଫେରନ୍ତୁ;

        AstClass* newClassp = srcClassp;
        ଯଦି (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ଅଚଳ typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        ଯଦି (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ସ୍ଥିର ତାର୍କିକ defaultParamsHaveTypeQueryOnParamType(ଅଚଳ AstClassRefDType* nodep) {
        ତାର୍କିକ found = ମିଥ୍ୟା;
        nodep->foreach([&](ଅଚଳ AstAttrOf* attrp) {
            ଯଦି (found || !attrp->attrType().isTypeQuery()) ଫେରନ୍ତୁ;
            ଅଚଳ AstRefDType* ଅଚଳ refp = VN_CAST(attrp->fromp(), RefDType);
            ଯଦି (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = ସତ୍ୟ;
        });
        ଫେରନ୍ତୁ found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    ତାର୍କିକ classTypeMatchesDefaultClone(ଅଚଳ Ast_ଗଣ୍ଠି_DType* exprp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ଅଚଳ auto* ଅଚଳ ପଦସମଷ୍ଟି_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ଅଚଳ auto* ଅଚଳ origClassRefp = VN_CAST(origp, ClassRefDType);
        ଯଦି (!ପଦସମଷ୍ଟି_Class_Refp || !origClassRefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        ଯଦି (defaultClonep && defaultClonep == ପଦସମଷ୍ଟି_Class_Refp->classp()) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        ଯଦି (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        ଯଦି (!origClassRefp->classp()->hasGParam()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (origClassRefp->classp()->origName() != ପଦସମଷ୍ଟି_Class_Refp->classp()->origName())
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        ଯଦି (!m_defaultCloneInProgress.ଭର୍ତ୍ତି(origClassRefp->classp()).second) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ଅଚଳ origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ଅଚଳ ତାର୍କିକ match = resolvedModp && VN_CAST(resolvedModp, Class) == ପଦସମଷ୍ଟି_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ବୃକ୍ଷ(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        ଫେରନ୍ତୁ match;
    }

    ସ୍ଥିର ତାର୍କିକ paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        ଯଦି (exprp->ଅଙ୍କ().width() == origp->ଅଙ୍କ().width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (exprp->ଅଙ୍କ().isOpaque()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (origp->ଅଙ୍କ().isOpaque()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxWidth = std::max(exprp->ଅଙ୍କ().width(), origp->ଅଙ୍କ().width());
        V3_ସଂଖ୍ୟା ପଦସମଷ୍ଟି_ଅଙ୍କ{exprp, maxWidth};
        ଯଦି (exprp->ଅଙ୍କ().isSigned()) {
            ପଦସମଷ୍ଟି_ଅଙ୍କ.opExtendS(exprp->ଅଙ୍କ(), exprp->ଅଙ୍କ().width());
        } ଅନ୍ୟଥା {
            ପଦସମଷ୍ଟି_ଅଙ୍କ.opAssign(exprp->ଅଙ୍କ());
        }
        V3_ସଂଖ୍ୟା orig_ଅଙ୍କ{origp, maxWidth};
        ଯଦି (origp->ଅଙ୍କ().isSigned()) {
            orig_ଅଙ୍କ.opExtendS(origp->ଅଙ୍କ(), origp->ଅଙ୍କ().width());
        } ଅନ୍ୟଥା {
            orig_ଅଙ୍କ.opAssign(origp->ଅଙ୍କ());
        }
        V3_ସଂଖ୍ୟା isEq{exprp, 1};
        isEq.opEq(ପଦସମଷ୍ଟି_ଅଙ୍କ, orig_ଅଙ୍କ);
        ଫେରନ୍ତୁ isEq.isNeqZero();
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
    ଶୂନ୍ୟ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (!pinp->exprp()) ଜାରି;
            ଯଦି (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ଜାରି;
            ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fwdRefp = VN_CAST(pinp->exprp(), ଗଣ୍ଠି_Var_Ref);
            ଯଦି (!fwdRefp) ଜାରି;
            ଅଚଳ AstIfaceRefDType* ଅଚଳ resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ଅଚଳ newIrefp = ନୂତନ AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            ଯଦି (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ));
            }
            pinp->exprp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    ଶୂନ୍ୟ name_By_Pin_ମୂଲ୍ୟ(AstPin* pinp, Ast_ଗଣ୍ଠି_Module* srcModp, AstVar* modvarp, string& longnamer,
                        ତାର୍କିକ& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_ମୂଲ୍ୟ_ସଂଖ୍ୟା(pinp->exprp());
        any_overridesr = ସତ୍ୟ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    ଶୂନ୍ୟ cellPinCleanup(Ast_ଗଣ୍ଠି* nodep, AstPin* pinp, AstPin* paramsp, Ast_ଗଣ୍ଠି_Module* srcModp,
                        string& longnamer, ତାର୍କିକ& any_overridesr,
                        std::unordered_set<ଅଚଳ AstPin*>& overridingTypePinsr) {
        ଯଦି (!pinp->exprp()) ଫେରନ୍ତୁ;  // No-connect
        ଯଦି (AstVar* ଅଚଳ modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            ଯଦି (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ଅନ୍ୟଥା ଯଦି (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_ମୂଲ୍ୟ(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } ଅନ୍ୟଥା {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                ଯଦି (is_Aggregate_Param_ମୂଲ୍ୟ(pinp->exprp())) {
                    name_By_Pin_ମୂଲ୍ୟ(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    ଫେରନ୍ତୁ;
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
                ଯଦି (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    ଅଚଳ ତାର୍କିକ dependent = modvarp->valuep()->exists(
                        [](Ast_ଗଣ୍ଠି* np) { ଫେରନ୍ତୁ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    ଯଦି (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                ଯଦି (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ଅଚଳ exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ଅଚଳ origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                ଯଦି (exprp && !exprp->ଅଙ୍କ().isDouble() && !exprp->ଅଙ୍କ().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    ତାର୍କିକ cloneVarpUnresolved = ମିଥ୍ୟା;
                    ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_ବୃକ୍ଷ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        ଯଦି (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                        cloneVarp->childDTypep(origDTypep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxSubstIters = 1000;
                        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା it = 0; it < maxSubstIters; ++it) {
                            ତାର୍କିକ any = ମିଥ୍ୟା;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ଅଚଳ targetp = varrefp->varp();
                                Ast_ଗଣ୍ଠି* replacep = nullptr;
                                ପାଇଁ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ଯଦି (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        ଯଦି (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                                        }
                                        ଭାଙ୍ଗ;
                                    }
                                }
                                ଯଦି (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ଅଚଳ patp = VN_CAST(replacep, Pattern);
                                ଯଦି (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                                }
                                ଯଦି (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ବୃକ୍ଷ(), varrefp);
                                    any = ସତ୍ୟ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_ଗଣ୍ଠି_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ଅଚଳ ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                ଯଦି (!ptdp) ଫେରନ୍ତୁ;
                                AstPin* overridePinp = nullptr;
                                ପାଇଁ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ଯଦି (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        ଭାଙ୍ଗ;
                                    }
                                }
                                Ast_ଗଣ୍ଠି_DType* ଅଚଳ substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ଗଣ୍ଠି_DType)
                                                   : ptdp->subDTypep();
                                ଯଦି (substp) toReplace.emplace_back(refp, substp);
                            });
                            ପାଇଁ (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ଅଚଳ refp = it->first;
                                refp->replaceWith(it->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                                VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
                                any = ସତ୍ୟ;
                            }
                            ଯଦି (!any) ଭାଙ୍ଗ;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ଗଣ୍ଠି* np) {
                            ଯଦି (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = ସତ୍ୟ;
                                ଫେରନ୍ତୁ;
                            }
                            ଯଦି (AstEnumItemRef* ଅଚଳ itemRefp = VN_CAST(np, EnumItemRef)) {
                                ଯଦି (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = ସତ୍ୟ;
                                }
                                ଫେରନ୍ତୁ;
                            }
                            AstRefDType* ଅଚଳ refp = VN_CAST(np, RefDType);
                            ଯଦି (!refp) ଫେରନ୍ତୁ;
                            ଯଦି (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = ସତ୍ୟ;
                            }
                            // A type declared in the template must not be widthed there
                            ଯଦି (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = ସତ୍ୟ;
                            }
                        });
                    }
                    ଯଦି (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        ଯଦି (AstConst* ଅଚଳ widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            ଯଦି (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ବୃକ୍ଷ(), cloneVarp);
                }
                AstConst* ଅଚଳ namingExprp = normedNamep ? normedNamep : exprp;
                ଯଦି (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ଗଣ୍ଠି* ଅଚଳ pinExprp = pinp->exprp();
                    pinExprp->replaceWith(ନୂତନ AstConst{pinp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ବୃକ୍ଷ(), pinExprp);
                } ଅନ୍ୟଥା ଯଦି (origp
                           && (exprp->same_ବୃକ୍ଷ(origp)
                               || (exprp->ଅଙ୍କ().width() == origp->ଅଙ୍କ().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } ଅନ୍ୟଥା ଯଦି (namingExprp->ଅଙ୍କ().isDouble() || namingExprp->ଅଙ୍କ().isString()
                           || namingExprp->ଅଙ୍କ().isFourState()
                           || namingExprp->ଅଙ୍କ().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_ମୂଲ୍ୟ_ସଂଖ୍ୟା(namingExprp));
                    any_overridesr = ସତ୍ୟ;
                } ଅନ୍ୟଥା {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->ଅଙ୍କ().ascii(ମିଥ୍ୟା));
                    any_overridesr = ସତ୍ୟ;
                }
                ଯଦି (normedNamep) VL_DO_DANGLING(normedNamep->delete_ବୃକ୍ଷ(), normedNamep);
            }
        } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ଗଣ୍ଠି_DType* rawTypep = VN_CAST(pinp->exprp(), ଗଣ୍ଠି_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            ତାର୍କିକ skipWidthForTemplateStruct = ମିଥ୍ୟା;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                ଯଦି (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    ଯଦି (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = ସତ୍ୟ;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                ଯଦି (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_ଗଣ୍ଠି_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* origp = modvarp->skipRefToNonRefp();
            ଯଦି (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ଅନ୍ୟଥା ଯଦି (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } ଅନ୍ୟଥା {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                ଯଦି (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ଗଣ୍ଠି_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                ଯଦି (!modvarp->fwdType().is_ଗଣ୍ଠି_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                ଯଦି (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } ଅନ୍ୟଥା {
                    ଯଦି (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ଗଣ୍ଠି_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    ଯଦି (AstClassRefDType* ଅଚଳ classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        ଯଦି (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } ଅନ୍ୟଥା {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    ଶୂନ୍ୟ cellInterfaceCleanup(AstPin* pinsp, Ast_ଗଣ୍ଠି_Module* srcModp, string& longnamer,
                              ତାର୍କିକ& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        ପାଇଁ (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଅଚଳ AstVar* ଅଚଳ modvarp = pinp->modVarp();
            ଯଦି (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ଜାରି;
            }
            ଯଦି (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ଅଚଳ portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ exprp = pinp->exprp();
                ଯଦି (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ଜାରି;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrp = VN_CAST(exprp, ଗଣ୍ଠି_Var_Ref)) {
                    ଯଦି (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                ଯଦି (!pinIrefp && exprp) {
                    ଅଚଳ Ast_ଗଣ୍ଠି* basep = exprp;
                    ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ selp = VN_CAST(basep, ଗଣ୍ଠି_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vrp = VN_CAST(basep, VarRef)) {
                        ଯଦି (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                ଯଦି (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } ଅନ୍ୟଥା ଯଦି (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } ଅନ୍ୟଥା {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    ଯଦି (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_ମୂଲ୍ୟ_ସଂଖ୍ୟା(pinIrefp));
                        any_overridesr = ସତ୍ୟ;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        ଯଦି (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ଗଣ୍ଠି::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ଗଣ୍ଠି::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ storeOriginalParams(Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ଗଣ୍ଠି* originalParamp = nullptr;
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isGParam()) originalParamp = varp->clonep();
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ଯଦି (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ଯଦି (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    ଶୂନ୍ୟ genericInterfaceVarSetup(ଅଚଳ AstPin* ଅଚଳ paramsp, ଅଚଳ AstPin* ଅଚଳ pinsp) {
        std::unordered_map<string, ଅଚଳ AstPin*> paramspMap;
        ପାଇଁ (ଅଚଳ AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ଭର୍ତ୍ତି({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        ଯଦି (paramspMap.empty()) ଫେରନ୍ତୁ;

        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstPin* ଅଚଳ pinp = VN_CAST(nodep, Pin)) {
                ଯଦି (AstVar* ଅଚଳ varp = pinp->modVarp()) {
                    Ast_ଗଣ୍ଠି_DType* dtypep = varp->childDTypep()->elemDTypep();
                    ଯଦି (AstIfaceGenericDType* ଅଚଳ ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ଅଚଳ auto iter = paramspMap.find(varp->name());
                        ଯଦି (iter == paramspMap.end()) ଜାରି;
                        Ast_ଗଣ୍ଠି* ଅଚଳ backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ଅଚଳ AstPin* ଅଚଳ paramp = iter->second;
                        paramspMap.erase(iter);
                        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ଅଚଳ newIfacerefp = ନୂତନ AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        ଯଦି (auto* ଅଚଳ arrDtp = VN_CAST(backp, ଗଣ୍ଠି_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } ଅନ୍ୟଥା ଯଦି (auto* ଅଚଳ arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } ଅନ୍ୟଥା {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ଠେଲ_Deletep(ifaceGDTypep), ifaceGDTypep);
                        ଯଦି (paramspMap.empty()) ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    ଶୂନ୍ୟ resolveDefaultParams(Ast_ଗଣ୍ଠି* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        ଯଦି (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            ଯଦି (!classp) ଫେରନ୍ତୁ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } ଅନ୍ୟଥା ଯଦି (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                ଯଦି (!paramTypep->isGParam()) ଜାରି;
                m_paramIndex.emplace(paramTypep, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_ଗଣ୍ଠି_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        ପାଇଁ (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* ଅଚଳ paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ଅଚଳ reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            ଯଦି (!reqDtp) ଜାରି;

            // If default is a reference to another param type, record dependency
            ଯଦି (AstRefDType* ଅଚଳ refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                ଯଦି (AstParamTypeDType* ଅଚଳ sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    ଯଦି (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    ଜାରି;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(reqDtp->lhsp(), ଗଣ୍ଠି_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ଅଚଳ typep = pinp->modPTypep();
            ଅଚଳ auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            ଯଦି (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } ଅନ୍ୟଥା {
                pinsByIndex.ଠେଲ_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        ପାଇଁ (ତାର୍କିକ resolvedSome = ସତ୍ୟ; resolvedSome;) {
            resolvedSome = ମିଥ୍ୟା;
            ପାଇଁ (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                ଯଦି (pinsByIndex[paramIdx]) ଜାରି;
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                ଯଦି (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                ଯଦି (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = defaultTypeNodes[paramIdx];
                    newPinp = ନୂତନ AstPin{dtypep->fileline(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                }

                ଯଦି (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(ସତ୍ୟ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    ଯଦି (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } ଅନ୍ୟଥା ଯଦି (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    ଯଦି (!paramsp) paramsp = newPinp;
                    resolvedSome = ସତ୍ୟ;
                }
            }
        }
    }

    Ast_ଗଣ୍ଠି_Module* ଗଣ୍ଠି_Deparam_Common(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, ତାର୍କିକ any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        ଯଦି (debug() >= 9 && paramsp) paramsp->dump_ବୃକ୍ଷ_And_Next(cout, "-  cellparams: ");

        ଯଦି (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } ଅନ୍ୟଥା {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ଅଚଳ AstPin*> overridingTypePins;
            ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (!pinp->exprp()) ଜାରି;  // An empty override keeps the default
                ଯଦି (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ଜାରି;
                }
                AstVar* ଅଚଳ modvarp = pinp->modVarp();
                ଯଦି (!modvarp || !modvarp->isGParam()) ଜାରି;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ଅଚଳ AstPattern* ଅଚଳ patternp = VN_CAST(pinp->exprp(), Pattern);
                ଯଦି (patternp && !patternp->childDTypep()) ଜାରି;
                ଯଦି (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_ମୂଲ୍ୟ(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (AstParamTypeDType* ଅଚଳ modptp = pinp->modPTypep()) {
                    ଯଦି (!overridingTypePins.ଗଣନା_2(pinp)) ଜାରି;
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ typep = VN_AS(pinp->exprp(), ଗଣ୍ଠି_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_ମୂଲ୍ୟ_ସଂଖ୍ୟା(typep->skipRefToNonRefp());
                    any_overrides = ସତ୍ୟ;
                    ଜାରି;
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
        ତାର୍କିକ defaultsResolved = ମିଥ୍ୟା;
        ଯଦି (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            ପାଇଁ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ଯଦି (pinp->modPTypep()) {
                    any_overrides = ସତ୍ୟ;
                    defaultsResolved = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
            }
        }
        ଯଦି (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = ସତ୍ୟ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ଗଣ୍ଠି_Module* newModp = nullptr;
        ଯଦି (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            ଯଦି (!paramedModp) ଫେରନ୍ତୁ nullptr;  // Unsupported, already reported
            paramedModp->dead(ମିଥ୍ୟା);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } ଅନ୍ୟଥା ଯଦି (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            ଯଦି (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ଗଣ୍ଠି_Module* ଗଣ୍ଠି_Copyp = srcModp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ଗଣ୍ଠି_Copyp);
                m_deleter.ଠେଲ_Deletep(ଗଣ୍ଠି_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ଗଣ୍ଠି_Copyp);
                storeOriginalParams(ଗଣ୍ଠି_Copyp);
            }
            newModp = srcModp;
        } ଅନ୍ୟଥା {
            ଅଚଳ string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ଅଚଳ ModInfo* ଅଚଳ modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            ଯଦି (!modInfop) ଫେରନ୍ତୁ nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ଅଚଳ ତାର୍କିକ cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ଅଚଳ newClassp = VN_CAST(newModp, Class);
        ଯଦି (newClassp && !newClassp->user2()) m_specializedClassps.ଠେଲ_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        ଯଦି (defaultsResolved) srcModp->user4p(newModp);

        ପାଇଁ (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                ଯଦି (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        ଯଦି (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ(), paramsp);
        ଫେରନ୍ତୁ newModp;
    }

    Ast_ଗଣ୍ଠି_Module* cellDeparam(AstCell* nodep, Ast_ଗଣ୍ଠି_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp = ଗଣ୍ଠି_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        ଯଦି (!newModp) ଫେରନ୍ତୁ nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(ମିଥ୍ୟା);
        ଫେରନ୍ତୁ newModp;
    }
    Ast_ଗଣ୍ଠି_Module* ifaceRefDeparam(AstIfaceRefDType* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        ଯଦି (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ଅଚଳ string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ଅଚଳ string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ଅଚଳ size_t ownerPos = ownerBaseName.find("__");
            ଯଦି (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ଅଚଳ size_t srcPos = srcBaseName.find("__");
            ଯଦି (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            ଯଦି (ownerBaseName == srcBaseName) {
                ତାର୍କିକ allOwnParams = ସତ୍ୟ;
                ପାଇଁ (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    ଯଦି (AstRefDType* ଅଚଳ refp = VN_CAST(pinp->exprp(), RefDType)) {
                        ଯଦି (AstParamTypeDType* ଅଚଳ ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ଗଣ୍ଠି_Module* ଅଚଳ ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            ଯଦି (ptdOwnerp != m_modp) allOwnParams = ମିଥ୍ୟା;
                        } ଅନ୍ୟଥା {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } ଅନ୍ୟଥା {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                ଯଦି (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    ଯଦି (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
                    ଫେରନ୍ତୁ ownerIfacep;
                }
            }
        }

        Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
            = ଗଣ୍ଠି_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ମିଥ୍ୟା);
        ଯଦି (!newModp) ଫେରନ୍ତୁ nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        ଫେରନ୍ତୁ newModp;
    }
    Ast_ଗଣ୍ଠି_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ଗଣ୍ଠି_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
            = ଗଣ୍ଠି_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ମିଥ୍ୟା);
        ଯଦି (!newModp) ଫେରନ୍ତୁ nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ଅଚଳ refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ଅଚଳ newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        ଯଦି (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            ଯଦି (AstTypedef* ଅଚଳ typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        ଫେରନ୍ତୁ newModp;
    }
    Ast_ଗଣ୍ଠି_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ଗଣ୍ଠି_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
            = ଗଣ୍ଠି_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ମିଥ୍ୟା);
        ଯଦି (!newModp) ଫେରନ୍ତୁ nullptr;
        AstClass* ଅଚଳ newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        ଫେରନ୍ତୁ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    ଶ୍ରେଣୀ DeferredResolverVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ଅଚଳ Ast_ଗଣ୍ଠି*> m_reachedDecls;

        ତାର୍କିକ firstReach(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ declp) { ଫେରନ୍ତୁ m_reachedDecls.ଭର୍ତ୍ତି(declp).second; }

        ଶୂନ୍ୟ iterateParamType(AstParamTypeDType* ଅଚଳ paramTypep) {
            ଯଦି (firstReach(paramTypep)) iterate(paramTypep);
        }
        ଶୂନ୍ୟ iterateTypedef(AstTypedef* ଅଚଳ tdefp) {
            ଯଦି (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
        ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ଗଣ୍ଠି* ଅଚଳ targetp = nodep->classOrPackageNodep();
            ଯଦି (AstTypedef* ଅଚଳ tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        ଶୂନ୍ୟ visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ଅଚଳ tdefp = nodep->typedefp();
            ଯଦି (tdefp) {
                iterateTypedef(tdefp);
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ଅଚଳ varp = nodep->varp();
            ଯଦି (firstReach(varp)) {
                iterate(varp->subDTypep());
                ଅଚଳ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                ଯଦି (varp->varType() == VVarType::LPARAM && deferredVarps.ଗଣନା_2(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    ସର୍ବସାଧାରଣ:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ଗଣ୍ଠି* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

ସର୍ବସାଧାରଣ:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    ଶୂନ୍ୟ retargetIfaceRefs(Ast_ଗଣ୍ଠି_Module* parentModp, ଅଚଳ string& cellName) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        ଯଦି (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) ଫେରନ୍ତୁ;
        parentModp->foreach([&](AstRefDType* refp) {
            ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            ଯଦି (!tagp || tagp->m_cellPath != cellName) ଫେରନ୍ତୁ;
            // Only retarget REFDTYPEs owned by parentModp.
            ଯଦି (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) ଫେରନ୍ତୁ;
            ଯଦି (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    ଶୂନ୍ୟ resolveDeferredDotsReachableFrom(Ast_ଗଣ୍ଠି* rootp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ଏହି, rootp};
    }

    Ast_ଗଣ୍ଠି_Module* ଗଣ୍ଠି_Deparam(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_Module* srcModp, Ast_ଗଣ୍ଠି_Module* modp,
                               ଅଚଳ string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ଗଣ୍ଠି_Name = nodep->name();
        ଯଦି (AstIfaceRefDType* ଅଚଳ ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            ଯଦି (ଗଣ୍ଠି_Name.empty()) ଗଣ୍ଠି_Name = ifaceRefp->cellName();
        }
        ଅଚଳ string instanceName
            = ଗଣ୍ଠି_Name.empty() ? someInstanceName : (someInstanceName + "." + ଗଣ୍ଠି_Name);

        ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(nodep, Cell)) {
            ପାଇଁ (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ଅଚଳ nextp = VN_AS(pinp->nextp(), Pin);
                ଯଦି (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_ବୃକ୍ଷ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ଗଣ୍ଠି_Module* newModp = nullptr;
        ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } ଅନ୍ୟଥା ଯଦି (AstIfaceRefDType* ଅଚଳ ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } ଅନ୍ୟଥା ଯଦି (AstClassRefDType* ଅଚଳ classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ଅନ୍ୟଥା ଯଦି (AstClassOrPackageRef* ଅଚଳ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        ଯଦି (!newModp) ଫେରନ୍ତୁ nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        ଫେରନ୍ତୁ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        ଫେରନ୍ତୁ taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
            m_allModuleNames.ଭର୍ତ୍ତି(modp->name());
        }
    }
    ~ParamProcessor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

ଶ୍ରେଣୀ ClassRefUnlinkerVisitor final : ସର୍ବସାଧାରଣ VNVisitor {

ସର୍ବସାଧାରଣ:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        ଯଦି (nodep->paramsp()) {
            ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                ଯଦି (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ଅଚଳ dotp = VN_CAST(nodep->backp(), Dot);
                    ଯଦି (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(ଠେଲ_Deletep(dotp), dotp, nodep);
                    } ଅନ୍ୟଥା {
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {}  // don't iterate inside classes
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

ଶ୍ରେଣୀ ParamClassRefDTypeRelinkVisitor final : ସର୍ବସାଧାରଣ VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ଅଚଳ AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ଗଣ୍ଠି_Module* m_ownerModp = nullptr;
    Ast_ଗଣ୍ଠି_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    ଶୂନ୍ୟ ensureOwnerMap() {
        ଯଦି (m_ownerMapForp == m_ownerModp) ଫେରନ୍ତୁ;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        ଯଦି (!m_ownerModp) ଫେରନ୍ତୁ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        ଯଦି (AstClass* ଅଚଳ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ଯଦି (ownerClassp->hasGParam()) ଫେରନ୍ତୁ;
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ଅଚଳ tdp = VN_CAST(sp, Typedef);
            ଯଦି (!tdp) ଜାରି;
            AstClassRefDType* ଅଚଳ crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            ଯଦି (!crdtp || !crdtp->classp()) ଜାରି;
            AstClass* ଅଚଳ targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            ଯଦି (targetp->hasGParam()) ଜାରି;
            ଅଚଳ std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ଅଚଳ auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            ଯଦି (!pair.second && pair.first->second != targetp) m_ambiguous.ଭର୍ତ୍ତି(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ସ୍ଥିର string classOrigName(ଅଚଳ AstClass* classp) {
        ଫେରନ୍ତୁ classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ସ୍ଥିର AstTypedef* findTypedefWithBases(AstClass* classp, ଅଚଳ string& name) {
        ପାଇଁ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ଯଦି (AstTypedef* ଅଚଳ tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                ଯଦି (tdp->subDTypep()) ଫେରନ୍ତୁ tdp;
            }
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    ଶୂନ୍ୟ retargetRefDType(AstRefDType* refp) {
        ଯଦି (!m_ownerModp) ଫେରନ୍ତୁ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        ଯଦି (AstClass* ଅଚଳ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ଯଦି (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ଅଚଳ AstClass* refClassp = nullptr;
                ଯଦି (AstTypedef* ଅଚଳ tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassOrPackageRef* ଅଚଳ classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                ଯଦି (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ଅଚଳ selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    ଯଦି (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        AstTypedef* ଅଚଳ oldTdp = refp->typedefp();
        ଯଦି (!oldTdp) ଫେରନ୍ତୁ;
        AstClass* ଅଚଳ oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        ଯଦି (!oldOwnerp) ଫେରନ୍ତୁ;

        ensureOwnerMap();
        ଯଦି (m_origNameToClone.empty()) ଫେରନ୍ତୁ;
        ଅଚଳ std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        ଯଦି (m_ambiguous.ଗଣନା_2(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ auto it = m_origNameToClone.find(origName);
        ଯଦି (it == m_origNameToClone.end()) ଫେରନ୍ତୁ;
        AstClass* ଅଚଳ correctClassp = it->second;
        ଯଦି (correctClassp == oldOwnerp) ଫେରନ୍ତୁ;
        AstTypedef* ଅଚଳ newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        ଯଦି (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            ଫେରନ୍ତୁ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        ଯଦି (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            ଫେରନ୍ତୁ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

ସର୍ବସାଧାରଣ:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstParamTypeDType* ଅଚଳ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        ଯଦି (AstParamTypeDType* ଅଚଳ paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ଅଚଳ auto it = m_paramTypeClassMap.find(paramtypep);
            ଯଦି (it != m_paramTypeClassMap.end()) {
                AstClass* ଅଚଳ origClassp = it->second;
                ଯଦି (origClassp->hasGParam() && !origClassp->user3p()) {
                    ଯଦି (AstClass* ଅଚଳ parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ଅଚଳ string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ଯଦି (AstParamTypeDType* ଅଚଳ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                ଯଦି (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    ଭାଙ୍ଗ;
                                }
                            }
                        }
                        ଯଦି (newParamTypep) {
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

ଶ୍ରେଣୀ ParamState final {
ସର୍ବସାଧାରଣ:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<ତାର୍କିକ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>;
    std::multimap<WQKey, Ast_ଗଣ୍ଠି_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ଗଣ୍ଠି_Module*, std::unordered_set<Ast_ଗଣ୍ଠି_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

ଶ୍ରେଣୀ ParamVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    ତାର୍କିକ m_iterateModule = ମିଥ୍ୟା;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<ତାର୍କିକ, Ast_ଗଣ୍ଠି*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ଅଚଳ AstRefDType*, ତାର୍କିକ>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    ତାର୍କିକ m_modIfaceRefsDone = ମିଥ୍ୟା;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    ତାର୍କିକ m_inGenerateCond = ମିଥ୍ୟା;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    ଶୂନ୍ୟ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ଗଣ୍ଠି_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = ସତ୍ୟ;

        // Visit all cells under module, recursively
        ଯେତେବେଳେ (ସତ୍ୟ) {
            // Classes specialized since the last pass still need their bodies elaborated
            ପାଇଁ (AstClass* ଅଚଳ classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{ସତ୍ୟ, classp->level()}, classp);
            }
            ଯଦି (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            ଯଦି (workQueue.empty()) ଭାଙ୍ଗ;

            ଅଚଳ auto itm = workQueue.cbegin();
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            ଯଦି (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                ଯଦି (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = ମିଥ୍ୟା;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            ଯେତେବେଳେ (!m_cellps.empty()) {
                ଅଚଳ auto itim = m_cellps.cbegin();
                Ast_ଗଣ୍ଠି* ଅଚଳ cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ଅଚଳ auto genHierIt = m_genHierNames.find(cellp);
                ଯଦି (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_ଗଣ୍ଠି_Module* srcModp = nullptr;
                ଯଦି (ଅଚଳ AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    ଯଦି (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } ଅନ୍ୟଥା {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                ଯଦି (!srcModp) ଜାରି;

                // Update path
                ଅଚଳ string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
                    = m_processor.ଗଣ୍ଠି_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    ଯଦି (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        ଯଦି (V3LinkDotIfaceCapture::enabled()) {
                            ଯଦି (ଅଚଳ AstCell* ଅଚଳ modCellp = VN_CAST(cellp, Cell)) {
                                ଯଦି (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ଭର୍ତ୍ତି(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    ଯଦି (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = ମିଥ୍ୟା;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ସ୍ଥିର string getRefBaseName(ଅଚଳ AstVarXRef* refp) {
        ଅଚଳ string dotted = refp->dotted();
        ଯଦି (dotted.empty()) ଫେରନ୍ତୁ "";
        ଫେରନ୍ତୁ dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    ଶୂନ୍ୟ logTemplateLeakRefs(Ast_ଗଣ୍ଠି_Module* parentModp, Ast_ଗଣ୍ଠି_Module* templateModp,
                             ଅଚଳ ଅକ୍ଷର* stage, Ast_ଗଣ୍ଠି* contextp) {
        ଯଦି (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) ଫେରନ୍ତୁ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା leak_ଗଣନା = 0;
        ଅଚଳ auto ancestryOf = [](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
            string ancestry;
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* curp = nodep; curp; curp = curp->backp()) {
                ଯଦି (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                ଯଦି (VN_IS(curp, ଗଣ୍ଠି_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    ଭାଙ୍ଗ;
                }
            }
            ଫେରନ୍ତୁ ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            ଯଦି (refp->typedefp()) {
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                ଯଦି (tdOwnerp == templateModp) {
                    ++leak_ଗଣନା;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            ଯଦି (refp->refDTypep()) {
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                ଯଦି (rdOwnerp == templateModp) {
                    ++leak_ଗଣନା;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            ଯଦି (!varrefp->varp()) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            ଯଦି (varOwnerp != templateModp) ଫେରନ୍ତୁ;
            ++leak_ଗଣନା;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ଗଣ୍ଠି::ଗଣ୍ଠି_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        ଯଦି (leak_ଗଣନା > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ଗଣନା);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    ଶୂନ୍ୟ checkParamNotHierRecurse(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ underTypeQuery = ମିଥ୍ୟା) {
        ପାଇଁ (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ଅଚଳ AstAttrOf* ଅଚଳ attrp = VN_CAST(nodep, AttrOf);
            ଅଚଳ ତାର୍କିକ childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            ଯଦି (ଅଚଳ AstVarXRef* ଅଚଳ refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                ତାର୍କିକ isIfaceRef = ମିଥ୍ୟା;
                ଯଦି (refp->varp() && refp->varp()->isIfaceParam()) {
                    ଅଚଳ string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.ଗଣନା_2(refname) || m_ifaceInstCells.ଗଣନା_2(refname));
                }
                ଯଦି (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ refp = VN_CAST(nodep, ଗଣ୍ଠି_FTask_Ref)) {
                ଯଦି (refp->dotted() != "") {
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
    ଶୂନ୍ୟ specializeNestedIfaceCells(Ast_ଗଣ୍ଠି_Module* ifaceModp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ଅଚଳ nestedCellp = VN_CAST(stmtp, Cell);
            ଯଦି (!nestedCellp) ଜାରି;
            ଯଦି (!VN_IS(nestedCellp->modp(), Iface)) ଜାରି;
            ଯଦି (!nestedCellp->paramsp()) ଜାରି;

            Ast_ଗଣ୍ଠି_Module* ଅଚଳ nestedSrcModp = nestedCellp->modp();
            ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ nestedNewModp = m_processor.ଗଣ୍ଠି_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                ଯଦି (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    ପାଇଁ (Ast_ଗଣ୍ଠି* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(sp, Var)) {
                            ଯଦି (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    ଯଦି (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    ତାର୍କିକ cellParamsReferenceIfacePorts(AstCell* cellp) {
        ଯଦି (!cellp->paramsp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (AstPin* ଅଚଳ pinp = VN_CAST(nodep, Pin)) {
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ exprp = pinp->exprp()) {
                    ଯଦି (ଅଚଳ AstVarXRef* ଅଚଳ refp = VN_CAST(exprp, VarXRef)) {
                        ଅଚଳ string refname = getRefBaseName(refp);
                        ଯଦି (!refname.empty()
                            && (m_ifacePortNames.ଗଣନା_2(refname)
                                || m_ifaceInstCells.ଗଣନା_2(refname)))
                            ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    ଶୂନ୍ୟ expandIfaceArrayPortDimensions(AstVar* portp, Ast_ଗଣ୍ଠି_DType* dtypep,
                                        ଅଚଳ std::string& suffix) {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        ଯଦି (!arrp) {
            AstVar* ଅଚଳ varp = portp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ oldp = varp->childDTypep()) {
                VL_DO_DANGLING(ଠେଲ_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            portp->addNextHere(varp);
            ଫେରନ୍ତୁ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା n : arrp->declRange().seq_ଡାହାଣ_To_ବାମ()) {
            ଅଚଳ std::string s = suffix + "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    ଶୂନ୍ୟ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ଅଚଳ AstRange* rangep, ଅଚଳ std::string& suffix, ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx) {
        // Base case: insert the element when no dimensions left
        ଯଦି (!rangep) {
            AstCell* ଅଚଳ elemp = arrayedCellp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            ଯଦି (ifaceVarp) {
                AstIfaceRefDType* ଅଚଳ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ଅଚଳ varp = ifaceVarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            ଫେରନ୍ତୁ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ଅଚଳ AstRange* ଅଚଳ subRangep = VN_AS(rangep->nextp(), Range);
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା n : rangep->seq_ଡାହାଣ_To_ବାମ()) {
            ଅଚଳ std::string s = suffix + "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    ଶୂନ୍ୟ visitCellOrClassRef(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        ଯଦି (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ଅଚଳ cellp = VN_CAST(nodep, Cell);
            ଯଦି (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ srcModp = cellp->modp();
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
                    = m_processor.ଗଣ୍ଠି_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                ଯଦି (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                ଯଦି (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        ଯଦି (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->recursiveClone()) nodep->dead(ସତ୍ୟ);  // Fake, made for recursive elimination
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            ଯଦି (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ଠେଲ_back(classp);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            ଫେରନ୍ତୁ;
        }

        // Start traversal at root-like things
        ଯଦି (nodep->is_ଶୀର୍ଷ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    ତାର୍କିକ isCircularType(ଅଚଳ AstRefDType* nodep) {
        ଅଚଳ auto iter = m_isCircular.emplace(nodep, ସତ୍ୟ);
        ଯଦି (!iter.second) ଫେରନ୍ତୁ iter.first->second;
        ଯଦି (ଅଚଳ AstRefDType* ଅଚଳ subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ଅଚଳ ତାର୍କିକ ret = isCircularType(subDTypep);
            iter.first->second = ret;
            ଫେରନ୍ତୁ ret;
        }
        iter.first->second = ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        ଅଚଳ ତାର୍କିକ isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        ଯଦି (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        ଯଦି (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } ଅନ୍ୟଥା ଯଦି (nodep->typedefp() && nodep->subDTypep()
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
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        ଯଦି (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            ପାଇଁ (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ଅଚଳ ifaceVarp = [&]() -> AstVar* {
                ଯଦି (!VN_IS(nodep->modp(), Iface)) ଫେରନ୍ତୁ nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ଅଚଳ varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                ଫେରନ୍ତୁ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            ଯଦି (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(ସତ୍ୟ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ଅଚଳ firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ଅଚଳ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            ଫେରନ୍ତୁ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        ଯଦି (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        ଯଦି (nodep->ifacep()) visitCellOrClassRef(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        ଯଦି (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, ମିଥ୍ୟା);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ସ୍ଥିର ଶୂନ୍ୟ gatherVars(Ast_ଗଣ୍ଠି* stmtsp, ତାର୍କିକ (*matchp)(ଅଚଳ AstVar*), VarsByName& varsr) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                ଯଦି (matchp(varp)) varsr.emplace(varp->name(), varp);
            } ଅନ୍ୟଥା ଯଦି (AstGenBlock* ଅଚଳ genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ଅଚଳ VarsByName& modIfaceRefs() {
        ଯଦି (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ଅଚଳ AstVar* varp) { ଫେରନ୍ତୁ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ m_modIfaceRefs;
    }
    ଅଚଳ VarsByName& ifaceParams(Ast_ଗଣ୍ଠି_Module* ifacep) {
        ଅଚଳ auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        ଯଦି (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ଅଚଳ AstVar* varp) { ଫେରନ୍ତୁ varp->isParam(); },
                pair.first->second);
        }
        ଫେରନ୍ତୁ pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ଗଣ୍ଠି* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ଅଚଳ lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ଅଚଳ rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        ଯଦି (!lhsp || !rhsp) ଫେରନ୍ତୁ nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        ଯଦି (VL_UNCOVERABLE(!m_modp)) ଫେରନ୍ତୁ nullptr;

        ଅଚଳ auto& refVars = modIfaceRefs();
        ଅଚଳ auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        ଯଦି (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) ଫେରନ୍ତୁ nullptr;
        AstVar* ଅଚଳ ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ଅଚଳ ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        ଯଦି (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) ଫେରନ୍ତୁ nullptr;

        ଅଚଳ auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ଅଚଳ auto targetVarIt = ifaceVars.find(rhsp->name());
        ଯଦି (targetVarIt == ifaceVars.end()) ଫେରନ୍ତୁ nullptr;
        AstVar* ଅଚଳ targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        ଯଦି (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        ଯଦି (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            ଫେରନ୍ତୁ nullptr;  // LCOV_EXCL_LINE
        }
        ଫେରନ୍ତୁ targetVarp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ resolveGenericIfaceDotsIn(Ast_ଗଣ୍ଠି* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ଠେଲ_back(dotp); });
        ପାଇଁ (AstDot* ଅଚଳ dotp : dotps) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ବୃକ୍ଷ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (nodep->user2SetOnce()) ଫେରନ୍ତୁ;  // Process once
        // Interface array port: split it into its elements
        ଯଦି (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(ସତ୍ୟ);
        }
        // Build cache of interface port names as we encounter them
        ଯଦି (nodep->isIfaceRef()) { m_ifacePortNames.ଭର୍ତ୍ତି(nodep->name()); }
        iterateChildren(nodep);
        ଯଦି (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            ଯଦି (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } ଅନ୍ୟଥା ଯଦି (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                ତାର୍କିକ hasUnresolvedLparamXRef = ମିଥ୍ୟା;
                nodep->valuep()->foreach([&](ଅଚଳ AstVarXRef* xrefp) {
                    ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = xrefp->varp()) {
                        ଯଦି (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = ସତ୍ୟ;
                        }
                    }
                });
                ଯଦି (hasUnresolvedLparamXRef) ଫେରନ୍ତୁ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ଅଚଳ ତାର୍କିକ hasDot = nodep->valuep()->exists([](AstDot*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
                ତାର୍କିକ refsDeferred = ମିଥ୍ୟା;
                ଯଦି (!hasDot) {
                    ଅଚଳ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ଅଚଳ AstVarRef* refp) {
                        ଯଦି (refsDeferred) ଫେରନ୍ତୁ;
                        AstVar* ଅଚଳ refVarp = refp->varp();
                        ଯଦି (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ଗଣନା_2(refVarp)) {
                            refsDeferred = ସତ୍ୟ;
                        }
                    });
                }
                ଯଦି (hasDot || refsDeferred) {
                    v3Global.rootp()->ଠେଲ_Deferred_Param_Varp(nodep);
                    ଫେରନ୍ତୁ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        ଯଦି (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        ଯଦି (nodep->varp()) iterate(nodep->varp());
    }
    ତାର୍କିକ ifaceParamReplace(AstVarXRef* nodep, Ast_ଗଣ୍ଠି* candp) {
        ପାଇଁ (; candp; candp = candp->nextp()) {
            ଯଦି (nodep->name() == candp->name()) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    ଯଦି (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPin* ଅଚଳ pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ deparamIfaceCellNow(AstCell* cellp) {
        ଯଦି (!cellp->paramsp()) ଫେରନ୍ତୁ;
        ଯଦି (!VN_IS(cellp->modp(), Iface)) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ srcModp = cellp->modp();
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ newModp
            = m_processor.ଗଣ୍ଠି_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        ଯଦି (newModp && newModp != srcModp) {
            ଯଦି (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        ଯଦି (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        ଯଦି (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            ଫେରନ୍ତୁ;
        }
        // Parameters of an interface array are accessed through its first element
        ଯଦି (nodep->varp() && nodep->varp()->isParam()) {
            ଅଚଳ auto it = m_ifaceInstCells.find(nodep->dotted());
            ଯଦି (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ଅଚଳ string dotted = nodep->dotted();
        ଯଦି (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ଅଚଳ Ast_ଗଣ୍ଠି* backp = nodep;
            ଯେତେବେଳେ ((backp = backp->backp())) {
                ଯଦି (VN_IS(backp, ଗଣ୍ଠି_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    ଭାଙ୍ଗ;
                }
                ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(backp, Var)) {
                    ଯଦି (!varp->isIfaceRef()) ଜାରି;
                    // Through all dimensions of an array
                    ଅଚଳ AstIfaceRefDType* ଅଚଳ ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    ଯଦି (!ifacerefp) ଜାରି;
                    // Interfaces passed in on the port map have ifaces
                    ଯଦି (ଅଚଳ AstIface* ଅଚଳ ifacep = ifacerefp->ifacep()) {
                        ଯଦି (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            ଯଦି (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    ଅନ୍ୟଥା ଯଦି (AstCell* ଅଚଳ cellp = ifacerefp->cellp()) {
                        ଯଦି (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            ଯଦି (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ଅଚଳ auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            ଯଦି (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ଅଚଳ cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    ଶୂନ୍ୟ visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ଅଚଳ AstClass* lhsClassp = nullptr;
        ଅଚଳ AstClassOrPackageRef* ଅଚଳ classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        ଯଦି (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ଗଣ୍ଠି* rhsDefp = nullptr;
        AstClassOrPackageRef* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        ଯଦି (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        ଯଦି (lhsClassp && rhsDefp) {
            m_state.m_dots.ଠେଲ_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } ଅନ୍ୟଥା {
            iterate(nodep->rhsp());
        }
    }

    ଶୂନ୍ୟ visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ଅଚଳ varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ taskrefp = VN_CAST(nodep->refp(), ଗଣ୍ଠି_FTask_Ref);
        AstEnumItemRef* ଅଚଳ enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        ଯଦି (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } ଅନ୍ୟଥା ଯଦି (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } ଅନ୍ୟଥା ଯଦି (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            ଫେରନ୍ତୁ;
        }
        iterate(nodep->cellrefp());

        ଯଦି (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } ଅନ୍ୟଥା ଯଦି (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } ଅନ୍ୟଥା {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        ପାଇଁ (Ast_ଗଣ୍ଠି *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> indices;
        ପାଇଁ (Ast_ଗଣ୍ଠି* s = nodep->selp(); s; s = s->nextp()) {
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(s, Const);
            ଯଦି (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                ଫେରନ୍ତୁ;
            }
            indices.ଠେଲ_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ଅଚଳ string viftopSuffix = "__Viftop";
        ଅଚଳ string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        ଅଚଳ string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        ଯଦି (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            ଫେରନ୍ତୁ;
        }
        pos += baseName.length();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx : indices) {
            ଅଚଳ string replacement = "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(idx) + "__KET__";
            ଯଦି (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                ଫେରନ୍ତୁ;
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
    ଶୂନ୍ୟ iterateGenerateCond(Ast_ଗଣ୍ଠି* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = ସତ୍ୟ;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    ଶୂନ୍ୟ visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->condp(), Const)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } ଅନ୍ୟଥା {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            // Normal edit rules will now recurse the replacement
        } ଅନ୍ୟଥା {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        ଯଦି (AstGenFor* ଅଚଳ forp = VN_AS(nodep->genforp(), GenFor)) {
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
            ଅଚଳ string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } ଅନ୍ୟଥା {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    ଶୂନ୍ୟ visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        ତାର୍କିକ hit = ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        ପାଇଁ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ଅଚଳ AstConst* ଅଚଳ exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        ପାଇଁ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* ep = itemp->condsp(); ep;) {
                Ast_ଗଣ୍ଠି* ଅଚଳ nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        ପାଇଁ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ଯଦି (!itemp->isDefault()) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    ଯଦି (ଅଚଳ AstConst* ଅଚଳ ccondp = VN_CAST(ep, Const)) {
                        V3_ସଂଖ୍ୟା match{nodep, 1};
                        match.opEq(ccondp->ଅଙ୍କ(), exprp->ଅଙ୍କ());
                        ଯଦି (!hit && match.isNeqZero()) {
                            hit = ସତ୍ୟ;
                            keepp = itemp->itemsp();
                        }
                    } ଅନ୍ୟଥା {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        ପାଇଁ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ଯଦି (itemp->isDefault()) {
                ଯଦି (!hit) {
                    hit = ସତ୍ୟ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        ଯଦି (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

ଶ୍ରେଣୀ Param_ଶୀର୍ଷ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    ଶୂନ୍ୟ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        ପାଇଁ (AstDot* ଅଚଳ dotp : m_state.m_dots) {
            ଅଚଳ AstClassOrPackageRef* ଅଚଳ classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ଅଚଳ AstClass* ଅଚଳ lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ଅଚଳ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            ପାଇଁ (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    ଭାଙ୍ଗ;
                }
            }
        }
    }

    ଶୂନ୍ୟ fixLevel(Ast_ଗଣ୍ଠି_Module* modp) {
        // Fix up level of module, based on who instantiates it
        ଯଦି (modp->user1SetOnce()) ଫେରନ୍ତୁ;  // Already fixed
        ଯଦି (m_state.m_parentps[modp].empty()) ଫେରନ୍ତୁ;  // Leave top levels alone
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxParentLevel = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        ଯଦି (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    ଶୂନ୍ୟ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_ଗଣ୍ଠି_Module*> modps;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module);
            modp->unlinkFrBack();
            modps.ଠେଲ_back(modp);
        }

        // Fix-up levels
        ଅଚଳ VNUser1InUse user1InUse;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ଅଚଳ Ast_ଗଣ୍ଠି_Module* ap, ଅଚଳ Ast_ଗଣ୍ଠି_Module* bp) {
                             ଫେରନ୍ତୁ ap->level() < bp->level();
                         });

        // Re-insert modules
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp : modps) netlistp->addModulesp(modp);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit Param_ଶୀର୍ଷ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ଅଚଳ ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ଅଚଳ ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ଗଣ୍ଠି_FTask_Ref* ftaskrefp) {
            Ast_ଗଣ୍ଠି_FTask* ftaskp = ftaskrefp->taskp();
            ଯଦି (!ftaskp || !ftaskp->classMethod()) ଫେରନ୍ତୁ;
            ଅଚଳ std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ଅଚଳ auto ancestorClassOf = [](Ast_ଗଣ୍ଠି* startp) -> AstClass* {
                ପାଇଁ (Ast_ଗଣ୍ଠି* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    ଯଦି (AstClass* ଅଚଳ cp = VN_CAST(np, Class)) ଫେରନ୍ତୁ cp;
                }
                ଫେରନ୍ତୁ nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            ଯଦି (refClassp == ftaskrefp->classOrPackagep())
                ଫେରନ୍ତୁ;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            ଯଦି (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (classp->user3p()) ଫେରନ୍ତୁ;  // will not get removed, no need to relink
            AstClass* ଅଚଳ parametrizedClassp = VN_CAST(classp->user4p(), Class);
            ଯଦି (!parametrizedClassp) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ଗଣ୍ଠି_FTask* ftaskp) {
                ଯଦି (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଯଦି (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ଅଚଳ ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        ପାଇଁ (AstClass* ଅଚଳ classp : m_state.m_paramClasses) {
            ଯଦି (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                ଯଦି (V3Error::error_ଗଣନା()) ଜାରି;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ଠେଲ_Deletep(classp->unlinkFrBack()), classp);
            } ଅନ୍ୟଥା {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(ମିଥ୍ୟା);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ଗଣ୍ଠି::user3_Clear_ବୃକ୍ଷ();
        ପାଇଁ (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ଗଣ୍ଠି* ଅଚଳ nodep) { nodep->user3(ସତ୍ୟ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
            nodep->foreachLink([&](Ast_ଗଣ୍ଠି** ଅଚଳ linkpp, ଅଚଳ ଅକ୍ଷର* namep) {
                ଯଦି (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_ଶୀର୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(Param_ଶୀର୍ଷ);
};

//######################################################################
// Param class functions

ଶୂନ୍ୟ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    ଯଦି (dump_ବୃକ୍ଷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_ଶୀର୍ଷ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    ଯଦି (dump_ବୃକ୍ଷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ବୃକ୍ଷ("param", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    ପାଇଁ (AstVar* ଅଚଳ varp : rootp->deferredParamVarps()) {
        ଯଦି (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
