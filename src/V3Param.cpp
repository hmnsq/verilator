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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Param.h"

#ಅನ್ತರ್ಭಾವ "V3Case.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"
#ಅನ್ತರ್ಭಾವ "V3LinkDotIfaceCapture.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3Unroll.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <cstdlib>
#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ಸ್ಥಿರ AstClassRefDType* class_Ref_DType_Of_ಗಂಟು(Ast_ಗಂಟು* nodep) {
    ಯಾವತ್ (AstTypedef* ಧ್ರುವ tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(nodep, ಗಂಟು_DType);
    ಪ್ರತ್ಯಾಗಮ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

ವರ್ಗ ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ಧ್ರುವ V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ಧ್ರುವ std::string, Ast_ಗಂಟು_Module*>;
    using ParamConstMap = std::map<ಧ್ರುವ std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ಧ್ರುವ std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ಧ್ರುವ ತಾರ್ಕಿಕ m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<ಧ್ರುವ V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ಧ್ರುವ std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<ಧ್ರುವ AstVar*> m_untypedParams;

    // METHODS

ಸಾರ್ವಜನಿಕ:
    ParameterizedHierBlocks(ಧ್ರುವ V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        ಪ್ರತಿ (ಧ್ರುವ auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ಧ್ರುವ V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            ಪ್ರತಿ (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    ನೂತನ File_ಸಾಲು{File_ಸಾಲು::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ಧ್ರುವ ತಾರ್ಕಿಕ inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ಸೇರಿಸು({hierOpt.second.origName(), {}});
        }
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            ಯದಿ (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ಧ್ರುವ string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ಧ್ರುವ auto defParamIt = m_modParams.find(actualModName);
            ಯದಿ (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                        ಯದಿ (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ಧ್ರುವ AstBasicDType* ಧ್ರುವ dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        ಯದಿ (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    ತಾರ್ಕಿಕ hierSubRun() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierSubRun; }
    ತಾರ್ಕಿಕ isHierBlock(ಧ್ರುವ string& origName) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_ಗಂಟು_Module* findByParams(ಧ್ರುವ string& origName, AstPin* firstPinp,
                                ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ಧ್ರುವ std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        ಧ್ರುವ auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ಧ್ರುವ AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        ಪ್ರತಿ (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            ತಾರ್ಕಿಕ found = ಸತ್ಯ;
            size_t paramIdx = 0;
            ಧ್ರುವ ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            ಪ್ರತಿ (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (!pinp->exprp()) ಅನುವರ್ತ;
                ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ modvarp = pinp->modVarp()) {
                    AstConst* ಧ್ರುವ constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ಧ್ರುವ auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ಧ್ರುವ ತಾರ್ಕಿಕ untyped = m_untypedParams.ಎಣಿಕೆ(paramIt->second);
                    AstConst* ಧ್ರುವ defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    ಯದಿ (defValuep && areSame(constp, defValuep)) {
                        ಯದಿ (!untyped || constp->same_ಮೌಲ್ಯ_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ಅನುವರ್ತ;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ಧ್ರುವ auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    ಯದಿ (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = ಅಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                    ಯದಿ (untyped && !constp->same_ಮೌಲ್ಯ_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = ಅಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            ಯದಿ (found && paramIdx == hierIt->second->params().size()) ಭಙ್ಗ;
        }
        ಯದಿ (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ಗಂಟು::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ಧ್ರುವ HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ಧ್ರುವ auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        ಯದಿ (it == m_hierBlockMod.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ it->second;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        ಯದಿ (pinValuep->isString()) {
            ಪ್ರತ್ಯಾಗಮ pinValuep->ಅಂಕೆ().toString() == hierOptParamp->ಅಂಕೆ().toString();
        }

        ಯದಿ (hierOptParamp->isDouble()) {
            ದ್ವಿಗುಣ var;
            ಯದಿ (pinValuep->isDouble()) {
                var = pinValuep->ಅಂಕೆ().toDouble();
            } ಅನ್ಯಥಾ {  // Cast from integer to real
                V3_ಸಂಖ್ಯೆ var_ಅಂಕೆ{pinValuep, V3_ಸಂಖ್ಯೆ::Double{}, 0.0};
                var_ಅಂಕೆ.opIToRD(pinValuep->ಅಂಕೆ());
                var = var_ಅಂಕೆ.toDouble();
            }
            ಪ್ರತ್ಯಾಗಮ V3_ಸಂಖ್ಯೆ::epsilonEqual(var, hierOptParamp->ಅಂಕೆ().toDouble());
        } ಅನ್ಯಥಾ {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_ಸಂಖ್ಯೆ var_ಅಂಕೆ{pinValuep, hierOptParamp->ಅಂಕೆ().width()};
            ಯದಿ (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_ಅಂಕೆ.opRToIRoundS(pinValuep->ಅಂಕೆ());
            } ಅನ್ಯಥಾ ಯದಿ (pinValuep->isSigned()) {
                var_ಅಂಕೆ.opExtendS(pinValuep->ಅಂಕೆ(), pinValuep->ಅಂಕೆ().width());
            } ಅನ್ಯಥಾ {
                var_ಅಂಕೆ.opAssign(pinValuep->ಅಂಕೆ());
            }
            V3_ಸಂಖ್ಯೆ isEq{pinValuep, 1};
            isEq.opEq(var_ಅಂಕೆ, hierOptParamp->ಅಂಕೆ());
            ಪ್ರತ್ಯಾಗಮ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

ವರ್ಗ ParamProcessor final {
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
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser3InUse m_inuser3;
    ಧ್ರುವ VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು*, Ast_ಗಂಟು*>;
    ಸಂರಚನಾ ModInfo final {
        Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_ಗಂಟು_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ಧ್ರುವ std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ಧ್ರುವ std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ಧ್ರುವ std::string, std::string> m_ಮೌಲ್ಯ_Names;  // Parameter value text to its name
    std::set<std::string> m_ಮೌಲ್ಯ_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ಸ್ಥಿರ constexpr std::string::size_type DIGEST_DIGITS = 8;

    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_ಮೌಲ್ಯ_Map = std::map<std::string, Ast_ಗಂಟು*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ಗಂಟು_Module*, Default_ಮೌಲ್ಯ_Map> m_default_Parameter_ಮೌಲ್ಯಗಳು;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ಧ್ರುವ Ast_ಗಂಟು_Module*, ಧ್ರುವ AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, ಪೂರ್ಣಾಙ್ಕ>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, ಪೂರ್ಣಾಙ್ಕ> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ಧ್ರುವ AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ಸ್ಥಿರ ಶೂನ್ಯ makeSmallNames(Ast_ಗಂಟು_Module* modp) {
        std::vector<ಪೂರ್ಣಾಙ್ಕ> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isGParam() || varp->isIfaceRef()) {
                    ವರ್ಣ ch = varp->name()[0];
                    ch = std::toupper(ch);
                    ಯದಿ (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<ಪೂರ್ಣಾಙ್ಕ>(ch)] * 256 + ch);
                    usedLetter[static_cast<ಪೂರ್ಣಾಙ್ಕ>(ch)]++;
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ typep = VN_CAST(stmtp, ParamTypeDType)) {
                ಧ್ರುವ ವರ್ಣ ch = 'T';
                typep->user3(usedLetter[static_cast<ಪೂರ್ಣಾಙ್ಕ>(ch)] * 256 + ch);
                usedLetter[static_cast<ಪೂರ್ಣಾಙ್ಕ>(ch)]++;
            }
        }
    }
    ಸ್ಥಿರ string paramSmallName(Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು* varp) {
        ಯದಿ (varp->user3() <= 1) makeSmallNames(modp);
        ಪೂರ್ಣಾಙ್ಕ index = varp->user3() / 256;
        ಧ್ರುವ ವರ್ಣ ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        ಯಾವತ್ (index) {
            st += cvtToStr(static_cast<ವರ್ಣ>((index % 25) + 'A'));
            index /= 26;
        }
        ಪ್ರತ್ಯಾಗಮ st;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Aggregate_Param_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ಸ್ಥಿರ string param_ಮೌಲ್ಯ_String(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (ಧ್ರುವ AstRefDType* ಧ್ರುವ refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string ಕೀಲಿ = nodep->name();
        ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            ಯದಿ (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ಕೀಲಿ = ifrtp->cellp()->modp()->name();
            } ಅನ್ಯಥಾ ಯದಿ (ifrtp->ifacep()) {
                ಕೀಲಿ = ifrtp->ifacep()->name();
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtypep
                   = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಕೀಲಿ += " ";
            ಕೀಲಿ += dtypep->verilogKwd();
            ಕೀಲಿ += " {";
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memberp);
                ಕೀಲಿ += ";";
            }
            ಕೀಲಿ += "}";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberDType* ಧ್ರುವ dtypep = VN_CAST(nodep, MemberDType)) {
            ಕೀಲಿ += " ";
            ಕೀಲಿ += param_ಮೌಲ್ಯ_String(dtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ dtypep = VN_CAST(nodep, BasicDType)) {
            ಯದಿ (dtypep->isSigned()) ಕೀಲಿ += " signed";
            ಯದಿ (dtypep->isRanged()) {
                ಕೀಲಿ += "[" + cvtToStr(dtypep->ಎಡ()) + ":" + cvtToStr(dtypep->ಬಲ()) + "]";
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ dtypep = VN_CAST(nodep, PackArrayDType)) {
            ಕೀಲಿ += "[";
            ಕೀಲಿ += cvtToStr(dtypep->ಎಡ());
            ಕೀಲಿ += ":";
            ಕೀಲಿ += cvtToStr(dtypep->ಬಲ());
            ಕೀಲಿ += "] ";
            ಕೀಲಿ += param_ಮೌಲ್ಯ_String(dtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPattern* ಧ್ರುವ patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            ಕೀಲಿ += "'{";
            ಪ್ರತಿ (ಧ್ರುವ AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                ಯದಿ (memp->isDefault()) ಕೀಲಿ += "default";
                ಯದಿ (ಧ್ರುವ AstText* ಧ್ರುವ textp = VN_CAST(memp->keyp(), Text)) {
                    ಕೀಲಿ += textp->text();
                } ಅನ್ಯಥಾ ಯದಿ (memp->keyp()) {
                    ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memp->keyp());
                }
                ಕೀಲಿ += ":";
                ಯದಿ (memp->repp()) ಕೀಲಿ += param_ಮೌಲ್ಯ_String(memp->repp()) + "x";
                ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                    ಕೀಲಿ += param_ಮೌಲ್ಯ_String(valuep) + ";";
                }
                ಕೀಲಿ += ",";
            }
            ಕೀಲಿ += "}";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ಕೀಲಿ += "{";
            ಪ್ರತಿ (ಧ್ರುವ AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                ಕೀಲಿ += cvtToStr(itemp->index()) + ":" + param_ಮೌಲ್ಯ_String(itemp->valuep()) + ",";
            }
            ಯದಿ (consArrayp->defaultp()) {
                ಕೀಲಿ += "default:" + param_ಮೌಲ್ಯ_String(consArrayp->defaultp()) + ",";
            }
            ಕೀಲಿ += "}";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConsStruct* ಧ್ರುವ consp = VN_CAST(nodep, ConsStruct)) {
            ಕೀಲಿ += "{";
            ಪ್ರತಿ (ಧ್ರುವ AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                ಕೀಲಿ += param_ಮೌಲ್ಯ_String(itemp->valuep());
                ಕೀಲಿ += ",";
            }
            ಕೀಲಿ += "}";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            ಯದಿ (classRefp->classp()) {
                ಧ್ರುವ string& className = classRefp->classp()->name();
                ಧ್ರುವ string& origName = classRefp->classp()->origName();
                ಧ್ರುವ ತಾರ್ಕಿಕ isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                ಯದಿ (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } ಅನ್ಯಥಾ ಯದಿ (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ಕೀಲಿ = className;
                } ಅನ್ಯಥಾ {
                    // Unspecialized class with no params - use origName
                    ಕೀಲಿ = origName;
                }
            } ಅನ್ಯಥಾ {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ dtypep = VN_CAST(nodep, QueueDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[$";
            ಯದಿ (dtypep->boundConst()) ಕೀಲಿ += ":" + cvtToStr(dtypep->boundConst());
            ಕೀಲಿ += "]";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ dtypep = VN_CAST(nodep, DynArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[]";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ಕೀಲಿ_DTypep()) + "]";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstWildcardArrayDType* ಧ್ರುವ dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ಕೀಲಿ = paramElemString(dtypep->subDTypep()) + "$[*]";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstEnumDType* ಧ್ರುವ dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ಕೀಲಿ += " enum {";
            ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(itemp->valuep(), Const);
                ಕೀಲಿ += itemp->name() + "=" + (constp ? constp->ಅಂಕೆ().ascii() : "?") + ";";
            }
            ಕೀಲಿ += "}";
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = VN_CAST(nodep, ಗಂಟು_DType)) {
            ಕೀಲಿ += dtypep->prettyDTypeName(ಸತ್ಯ);
        }
        UASSERT_OBJ(!ಕೀಲಿ.empty(), nodep, "Parameter yielded no value string");
        ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ;
    }
    // As paramValueString(), for the element or key type of a container
    ಸ್ಥಿರ string paramElemString(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        ಯದಿ (classRefp && classRefp->paramsp()) ಪ್ರತ್ಯಾಗಮ classRefp->prettyDTypeName(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ param_ಮೌಲ್ಯ_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ಸ್ಥಿರ string digestSuffix(ಧ್ರುವ string& text, std::set<string>& usedr) {
        ಧ್ರುವ string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        ಯಾವತ್ (digits < hex.size() && !usedr.ಸೇರಿಸು(hex.substr(0, digits)).second) ++digits;
        ಪ್ರತ್ಯಾಗಮ hex.substr(0, digits);
    }
    string param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        ಯದಿ (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ಧ್ರುವ string text = param_ಮೌಲ್ಯ_String(nodep);
        ಧ್ರುವ auto pair = m_ಮೌಲ್ಯ_Names.emplace(text, "");
        ಯದಿ (pair.second) pair.first->second = "z" + digestSuffix(text, m_ಮೌಲ್ಯ_Suffixes);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }
    string moduleCalcName(ಧ್ರುವ Ast_ಗಂಟು_Module* srcModp, ಧ್ರುವ string& longname) {
        string newname = longname;
        ಯದಿ (longname.length() > 30) {
            ಧ್ರುವ auto pair = m_longMap.emplace(longname, "");
            ಯದಿ (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        ಪ್ರತ್ಯಾಗಮ newname;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* arraySubDTypep(Ast_ಗಂಟು_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        ಯದಿ (ಧ್ರುವ AstBracketArrayDType* ಧ್ರುವ adtypep = VN_CAST(nodep, BracketArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ adtypep->subDTypep();
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* arraySubDTypeDeepp(Ast_ಗಂಟು_DType* nodep) {
        ಯಾವತ್ (Ast_ಗಂಟು_DType* ಧ್ರುವ subp = arraySubDTypep(nodep)) nodep = subp;
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isString(Ast_ಗಂಟು_DType* nodep) {
        ಯದಿ (AstBasicDType* ಧ್ರುವ basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            ಪ್ರತ್ಯಾಗಮ basicp->isString();
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ collectPins(CloneMap* clonemapp, Ast_ಗಂಟು_Module* modp, ತಾರ್ಕಿಕ originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಧ್ರುವ Ast_ಗಂಟು* originalParamp = nullptr;
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ಯದಿ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ಯದಿ (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            ಯದಿ (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    ಶೂನ್ಯ relinkPins(ಧ್ರುವ CloneMap* clonemapp, AstPin* startpinp) {
        ಪ್ರತಿ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ಧ್ರುವ auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } ಅನ್ಯಥಾ ಯದಿ (pinp->modPTypep()) {
                ಧ್ರುವ auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } ಅನ್ಯಥಾ {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    ಶೂನ್ಯ relinkPinsByName(AstPin* startpinp, Ast_ಗಂಟು_Module* modp) {
        std::map<ಧ್ರುವ string, AstVar*> nameToPin;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        ಪ್ರತಿ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = pinp->modVarp()) {
                ಧ್ರುವ auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasDescendantDefparams(ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                       std::set<ಧ್ರುವ Ast_ಗಂಟು_Module*>& visited) {
        ಯದಿ (!visited.ಸೇರಿಸು(modp).second) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ಧ್ರುವ cellp = VN_CAST(stmtp, Cell);
            ಯದಿ (!cellp) ಅನುವರ್ತ;
            ಪ್ರತಿ (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
            ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ childModp = cellp->modp()) {
                ಯದಿ (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }
        visited.erase(modp);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasDescendantDefparams(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        std::set<ಧ್ರುವ Ast_ಗಂಟು_Module*> visited;
        ಪ್ರತ್ಯಾಗಮ hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ಧ್ರುವ AstPin* nestedDefparamp(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಧ್ರುವ auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        ಯದಿ (!pair.second) ಪ್ರತ್ಯಾಗಮ pair.first->second;  // Searched, or being searched
        ಧ್ರುವ AstPin* foundp = nullptr;
        modp->exists([&](ಧ್ರುವ AstCell* cellp) {
            ಪ್ರತಿ (ಧ್ರುವ AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (!pinp->paramPath().empty()) foundp = pinp;
            }
            ಯದಿ (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            ಪ್ರತ್ಯಾಗಮ foundp != nullptr;
        });
        pair.first->second = foundp;
        ಪ್ರತ್ಯಾಗಮ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    ಶೂನ್ಯ checkSupportedParam(Ast_ಗಂಟು_Module* modp, AstPin* pinp) ಧ್ರುವ {
        // ConsArray is not supported because that can not be set via -G
        // option.
        ಯದಿ (pinp->modVarp()) {
            ತಾರ್ಕಿಕ supported = ಅಸತ್ಯ;
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(pinp->exprp(), Const)) {
                supported = ಸತ್ಯ;
                ಯದಿ (constp->ಅಂಕೆ().isString()
                    && !V3HierBlock::stringParamPassable(constp->ಅಂಕೆ().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            ಯದಿ (!supported) {
                pinp->v3error(
                    Ast_ಗಂಟು::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstParamTypeDType* ಧ್ರುವ typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ಗಂಟು::prettyNameQ(modp->origName()));
        }
    }
    ತಾರ್ಕಿಕ moduleExists(ಧ್ರುವ string& modName) ಧ್ರುವ {
        ಯದಿ (m_allModuleNames.find(modName) != m_allModuleNames.end()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_modNameMap.find(modName) != m_modNameMap.end()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    string parameterizedHierBlockName(Ast_ಗಂಟು_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ಗಂಟು*> pins;

        AstPin* pinp = paramPinsp;
        ಯಾವತ್ (pinp) {
            checkSupportedParam(modp, pinp);
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = pinp->modVarp()) {
                ಯದಿ (!pinp->exprp()) ಅನುವರ್ತ;
                ಯದಿ (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ಗಂಟು_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ಧ್ರುವ auto pair = m_default_Parameter_ಮೌಲ್ಯಗಳು.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        ಯದಿ (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            ಯದಿ (ಧ್ರುವ AstPin* ಧ್ರುವ defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ಗಂಟು::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_ಮೌಲ್ಯ_Map params;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                    ಯದಿ (varp->isGParam()) {
                        AstConst* ಧ್ರುವ constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ಗಂಟು* ಧ್ರುವ dtypep = static_cast<Ast_ಗಂಟು*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ಧ್ರುವ auto paramsIt = pair.first;
        ಯದಿ (paramsIt->second.empty()) ಪ್ರತ್ಯಾಗಮ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        ಪ್ರತಿ (auto&& default_ಮೌಲ್ಯ : paramsIt->second) {
            ಧ್ರುವ auto pinIt = pins.find(default_ಮೌಲ್ಯ.first);
            // If the pin does not have a value assigned, use the default one.
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep = pinIt == pins.end() ? default_ಮೌಲ್ಯ.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_ಮೌಲ್ಯ.first + "=";
            // constp can be nullptr

            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->ಅಂಕೆ().ascii(ಅಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ವೃಕ್ಷ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ಧ್ರುವ auto iter = m_longMap.find(longname);
        ಯದಿ (iter != m_longMap.end()) ಪ್ರತ್ಯಾಗಮ iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ಸೇರಿಸು(longname);
        ಯಾವತ್ (ಸತ್ಯ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ಧ್ರುವ string hashStr = hashStrGen.digestHex();
            ಪ್ರತಿ (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                ಯದಿ (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                ಯದಿ (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    ಪ್ರತ್ಯಾಗಮ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ಸೇರಿಸು(V3Os::trueRandom(64));
        }
    }
    ಶೂನ್ಯ replaceRefsRecurse(Ast_ಗಂಟು* ಧ್ರುವ nodep, ಧ್ರುವ AstClass* ಧ್ರುವ oldClassp,
                            AstClass* ಧ್ರುವ newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(nodep, ClassRefDType)) {
            ಯದಿ (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } ಅನ್ಯಥಾ ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ಯದಿ (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } ಅನ್ಯಥಾ ಯದಿ (AstTypedef* ಧ್ರುವ typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            ಯದಿ (typedefp->subDTypep()) {
                ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    ಯದಿ (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskRefp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            ಯದಿ (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ oldTaskp = ftaskRefp->taskp()) {
                ಯದಿ (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        ಯದಿ (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        ಯದಿ (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        ಯದಿ (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        ಯದಿ (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        ಯದಿ (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    ವರ್ಗ VarXRefRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        Ast_ಗಂಟು_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ಗಂಟು_Module*> m_varModuleMap;  // Cache var->module lookups

    ಸಾರ್ವಜನಿಕ:
        explicit VarXRefRelinkVisitor(Ast_ಗಂಟು_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ಗುಪ್ತ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ಗಂಟು_Module* findVarModule(AstVar* varp) {
            ಧ್ರುವ auto it = m_varModuleMap.find(varp);
            ಯದಿ (it != m_varModuleMap.end()) ಪ್ರತ್ಯಾಗಮ it->second;
            Ast_ಗಂಟು_Module* varModp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು* np = varp; np; np = np->backp()) {
                ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(np, ಗಂಟು_Module)) {
                    varModp = modp;
                    ಭಙ್ಗ;
                }
            }
            m_varModuleMap[varp] = varModp;
            ಪ್ರತ್ಯಾಗಮ varModp;
        }

        ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
            AstVar* ಧ್ರುವ varp = nodep->varp();
            ಯದಿ (!varp) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ಧ್ರುವ string& dotted = nodep->dotted();
            ಯದಿ (dotted.empty()) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            ಧ್ರುವ size_t dotPos = dotted.find('.');
            ಧ್ರುವ string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            ಯದಿ (portName.empty()) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (AstVar* ಧ್ರುವ varChkp = VN_CAST(stmtp, Var)) {
                    ಯದಿ (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        ಭಙ್ಗ;
                    }
                }
            }
            ಯದಿ (!portVarp) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ಧ್ರುವ irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            ಯದಿ (!irefp) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            Ast_ಗಂಟು_Module* ಧ್ರುವ newIfacep = irefp->ifaceViaCellp();
            ಯದಿ (!newIfacep) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ಗಂಟು_Module* ಧ್ರುವ varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            ಯದಿ (varModp && varModp != newIfacep) {
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (AstVar* ಧ್ರುವ newVarp = VN_CAST(stmtp, Var)) {
                        ಯದಿ (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            ಭಙ್ಗ;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    ಶೂನ್ಯ fixupCrossModuleRefDTypes(Ast_ಗಂಟು_Module* newModp) {
        ಯದಿ (!V3LinkDotIfaceCapture::enabled()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        ಪೂರ್ಣಾಙ್ಕ fixed_ಎಣಿಕೆ = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            ಯದಿ (!tagp) ಪ್ರತ್ಯಾಗಮ;

            Ast_ಗಂಟು_Module* ಧ್ರುವ correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            ಯದಿ (!correctModp || correctModp->dead()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (correctModp->parameterizedTemplate()) ಪ್ರತ್ಯಾಗಮ;

            ತಾರ್ಕಿಕ fixed = ಅಸತ್ಯ;
            ಯದಿ (refp->typedefp()) {
                ಯದಿ (AstTypedef* ಧ್ರುವ newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    ಯದಿ (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = ಸತ್ಯ;
                }
            }
            ಯದಿ (!fixed && refp->refDTypep()) {
                ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = ಸತ್ಯ;
                }
            }
            ಯದಿ (fixed) ++fixed_ಎಣಿಕೆ;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_ಎಣಿಕೆ);
    }

    // Return true on success, false on error
    ತಾರ್ಕಿಕ deepCloneModule(Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು* ifErrorp, AstPin* paramsp,
                         ಧ್ರುವ string& newname, ಧ್ರುವ IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ಗಂಟು_Module* newModp;
        ಯದಿ (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ಗಂಟು_Module);
        } ಅನ್ಯಥಾ {
            newModp = srcModp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        ಯದಿ (AstIface* ಧ್ರುವ newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(ಸತ್ಯ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(ಅಸತ್ಯ);

        newModp->name(newname);
        newModp->user2(ಅಸತ್ಯ);  // We need to re-recurse this module once changed
        newModp->recursive(ಅಸತ್ಯ);
        newModp->recursiveClone(ಅಸತ್ಯ);
        newModp->hasGParam(ಅಸತ್ಯ);
        // Recursion may need level cleanups
        ಯದಿ (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        ಯದಿ ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_ವೃಕ್ಷ(), newModp);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        ಯದಿ (AstClass* ಧ್ರುವ newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ಗಂಟು* insertp = srcModp;
        ಯಾವತ್ (VN_IS(insertp->nextp(), ಗಂಟು_Module)
               && VN_AS(insertp->nextp(), ಗಂಟು_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ಧ್ರುವ auto iter = m_modNameMap.find(newname);
        CloneMap* ಧ್ರುವ clonemapp = &(iter->second.m_cloneMap);
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
        ತಾರ್ಕಿಕ needRelinkByName = ಅಸತ್ಯ;
        ಯದಿ (paramsp) {
            ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
        }
        ಯದಿ (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } ಅನ್ಯಥಾ {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        ಪ್ರತಿ (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ portIrefp = it->first;
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ pinIrefp = it->second;
            AstIfaceRefDType* ಧ್ರುವ cloneIrefp = portIrefp->clonep();
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
        ಯದಿ (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (pinp->exprp()) {
                ಯದಿ (AstVar* ಧ್ರುವ modvarp = pinp->modVarp()) {
                    Ast_ಗಂಟು* ಧ್ರುವ newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ಧ್ರುವ exprp = VN_CAST(newp, Const);
                    AstConst* ಧ್ರುವ origp = VN_CAST(modvarp->valuep(), Const);
                    ಧ್ರುವ ತಾರ್ಕಿಕ overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    ಯದಿ (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    modvarp->overriddenParam(overridden);
                } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ modptp = pinp->modPTypep()) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    ಯದಿ (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಧ್ರುವ ModInfo* moduleFindOrClone(Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು* ifErrorp, AstPin* paramsp,
                                     ಧ್ರುವ string& newname, ಧ್ರುವ IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        ಯದಿ (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } ಅನ್ಯಥಾ {
            ಯದಿ (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ಧ್ರುವ ModInfo* ಧ್ರುವ modInfop = &(it->second);
        ಪ್ರತ್ಯಾಗಮ modInfop;
    }

    ಶೂನ್ಯ convertToStringp(Ast_ಗಂಟು* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        ಯದಿ (constp && !constp->ಅಂಕೆ().isString()) {
            constp->replaceWith(
                ನೂತನ AstConst{constp->fileline(), AstConst::String{}, constp->ಅಂಕೆ().toString()});
            VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    ಶೂನ್ಯ constify_Member_ಮೌಲ್ಯ(AstVar* varp, std::set<AstVar*>& inProgress) {
        ಯದಿ (!varp->valuep() || VN_IS(varp->valuep(), Const)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!inProgress.ಸೇರಿಸು(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            varp->valuep(ನೂತನ AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            ಪ್ರತ್ಯಾಗಮ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ಧ್ರುವ AstVarRef* refp) {
            AstVar* ಧ್ರುವ refVarp = refp->varp();
            ಯದಿ (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ಸೇರಿಸು(refVarp);
            }
        });
        ಪ್ರತಿ (AstVar* ಧ್ರುವ sibp : siblings) constify_Member_ಮೌಲ್ಯ(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ಸ್ಥಿರ AstParseRef* memberParseRef(Ast_ಗಂಟು* nodep) {
        ಯದಿ (AstParseRef* ಧ್ರುವ refp = VN_CAST(nodep, ParseRef)) ಪ್ರತ್ಯಾಗಮ refp;
        Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Pre_Sel);
        ಪ್ರತ್ಯಾಗಮ selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ಸ್ಥಿರ ಶೂನ್ಯ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ selp = VN_AS(dotp->rhsp(), ಗಂಟು_Pre_Sel);
            ಯದಿ (AstAttrOf* ಧ್ರುವ attrp = selp->attrp()) {
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    VL_DO_DANGLING(oldFromp->delete_ವೃಕ್ಷ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ವೃಕ್ಷ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDeferredMemberBase(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (VN_IS(nodep, MemberSel)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Pre_Sel)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDTypep = selp->fromp()->dtypep();
            ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ಗಂಟು_Array_DType) : nullptr;
            ಯದಿ (arrayDTypep) {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = arrayDTypep->subDTypep();
                ಪ್ರತ್ಯಾಗಮ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ಗಂಟು_UOr_Struct_DType);
            }
            ಪ್ರತ್ಯಾಗಮ isDeferredMemberBase(selp->fromp());
        }
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->dtypep();
        ಪ್ರತ್ಯಾಗಮ dtypep && VN_IS(dtypep->skipRefOrNullp(), ಗಂಟು_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ಸ್ಥಿರ ಶೂನ್ಯ resolveMemberDot(AstDot* dotp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = VN_CAST(dotp->lhsp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        AstParseRef* ಧ್ರುವ memberRefp = memberParseRef(dotp->rhsp());
        ಯದಿ (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) ಪ್ರತ್ಯಾಗಮ;
        AstMemberSel* ಧ್ರುವ newp = ನೂತನ AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    ಶೂನ್ಯ resolveDotToTypedef(Ast_ಗಂಟು* exprp) {
        AstDot* ಧ್ರುವ dotp = VN_CAST(exprp, Dot);
        ಯದಿ (!dotp) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ಧ್ರುವ classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        ಯದಿ (!classRefp) {
            resolveMemberDot(dotp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstParseRef* ಧ್ರುವ parseRefp = memberParseRef(dotp->rhsp());
        ಯದಿ (!parseRefp) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ಧ್ರುವ paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        ಯದಿ (paramTypep) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = paramTypep->subDTypep();
            AstClassRefDType* ಧ್ರುವ classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            ಯದಿ (classRefDTypep) {
                AstClass* ಧ್ರುವ srcClassp = classRefDTypep->classp();
                ಯದಿ (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    ಯದಿ (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        ಯದಿ (classRefp->paramsp()) {
            AstClass* ಧ್ರುವ srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            ಯದಿ (srcClassp && srcClassp->hasGParam()) {
                ಯದಿ (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        ಯದಿ (AstClassRefDType* ಧ್ರುವ crp = class_Ref_DType_Of_ಗಂಟು(classRefp->classOrPackageNodep())) {
            ಯದಿ (AstClass* ಧ್ರುವ srcClassp = crp->classp()) {
                ಯದಿ (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    ಯದಿ (AstClass* ಧ್ರುವ newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        ಯದಿ (!lhsClassp) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು* ಧ್ರುವ memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        ಯದಿ (AstTypedef* ಧ್ರುವ tdefp = VN_CAST(memberp, Typedef)) {
            ಯದಿ (parseRefp != dotp->rhsp()) ಪ್ರತ್ಯಾಗಮ;
            AstRefDType* ಧ್ರುವ refp = ನೂತನ AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
        } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    ಶೂನ್ಯ substituteParamMember(AstDot* ಧ್ರುವ dotp, AstParseRef* ಧ್ರುವ memberRefp,
                               AstVar* ಧ್ರುವ varp) {
        ಯದಿ (!varp->isParam() || !varp->valuep()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_ಮೌಲ್ಯ(varp, inProgress);
        }
        AstConst* ಧ್ರುವ constp = VN_CAST(varp->valuep(), Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ;
        AstConst* ಧ್ರುವ newp = constp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    ಶೂನ್ಯ resolveParamClassRefDType(AstRefDType* refp) {
        ಯದಿ (refp->typedefp() || refp->refDTypep()) ಪ್ರತ್ಯಾಗಮ;

        AstClassOrPackageRef* ಧ್ರುವ classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        ಯದಿ (!classRefp) ಪ್ರತ್ಯಾಗಮ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        ಯದಿ (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ಗಂಟು* targetp = classRefp->classOrPackageNodep();
        ಯಾವತ್ (ಧ್ರುವ AstTypedef* ಧ್ರುವ tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtargetp = VN_CAST(targetp, ಗಂಟು_DType))
            targetp = dtargetp->skipRefOrNullp();
        ಯದಿ (!targetp) ಪ್ರತ್ಯಾಗಮ;
        AstClassRefDType* ಧ್ರುವ classRefDTypep = VN_CAST(targetp, ClassRefDType);
        ಯದಿ (!classRefDTypep) ಪ್ರತ್ಯಾಗಮ;
        srcClassp = classRefDTypep->classp();
        ಯದಿ (!srcClassp) ಪ್ರತ್ಯಾಗಮ;

        AstClass* newClassp = srcClassp;
        ಯದಿ (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ಧ್ರುವ typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        ಯದಿ (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ defaultParamsHaveTypeQueryOnParamType(ಧ್ರುವ AstClassRefDType* nodep) {
        ತಾರ್ಕಿಕ found = ಅಸತ್ಯ;
        nodep->foreach([&](ಧ್ರುವ AstAttrOf* attrp) {
            ಯದಿ (found || !attrp->attrType().isTypeQuery()) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ AstRefDType* ಧ್ರುವ refp = VN_CAST(attrp->fromp(), RefDType);
            ಯದಿ (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = ಸತ್ಯ;
        });
        ಪ್ರತ್ಯಾಗಮ found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    ತಾರ್ಕಿಕ classTypeMatchesDefaultClone(ಧ್ರುವ Ast_ಗಂಟು_DType* exprp, ಧ್ರುವ Ast_ಗಂಟು_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ಧ್ರುವ auto* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ಧ್ರುವ auto* ಧ್ರುವ origClassRefp = VN_CAST(origp, ClassRefDType);
        ಯದಿ (!ಅಭಿವ್ಯಕ್ತಿ_Class_Refp || !origClassRefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        ಯದಿ (defaultClonep && defaultClonep == ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        ಯದಿ (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        ಯದಿ (!origClassRefp->classp()->hasGParam()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (origClassRefp->classp()->origName() != ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp()->origName())
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        ಯದಿ (!m_defaultCloneInProgress.ಸೇರಿಸು(origClassRefp->classp()).second) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ಧ್ರುವ origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        Ast_ಗಂಟು_Module* ಧ್ರುವ resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ಧ್ರುವ ತಾರ್ಕಿಕ match = resolvedModp && VN_CAST(resolvedModp, Class) == ಅಭಿವ್ಯಕ್ತಿ_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ವೃಕ್ಷ(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        ಪ್ರತ್ಯಾಗಮ match;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        ಯದಿ (exprp->ಅಂಕೆ().width() == origp->ಅಂಕೆ().width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (exprp->ಅಂಕೆ().isOpaque()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (origp->ಅಂಕೆ().isOpaque()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maxWidth = std::max(exprp->ಅಂಕೆ().width(), origp->ಅಂಕೆ().width());
        V3_ಸಂಖ್ಯೆ ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ{exprp, maxWidth};
        ಯದಿ (exprp->ಅಂಕೆ().isSigned()) {
            ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ.opExtendS(exprp->ಅಂಕೆ(), exprp->ಅಂಕೆ().width());
        } ಅನ್ಯಥಾ {
            ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ.opAssign(exprp->ಅಂಕೆ());
        }
        V3_ಸಂಖ್ಯೆ orig_ಅಂಕೆ{origp, maxWidth};
        ಯದಿ (origp->ಅಂಕೆ().isSigned()) {
            orig_ಅಂಕೆ.opExtendS(origp->ಅಂಕೆ(), origp->ಅಂಕೆ().width());
        } ಅನ್ಯಥಾ {
            orig_ಅಂಕೆ.opAssign(origp->ಅಂಕೆ());
        }
        V3_ಸಂಖ್ಯೆ isEq{exprp, 1};
        isEq.opEq(ಅಭಿವ್ಯಕ್ತಿ_ಅಂಕೆ, orig_ಅಂಕೆ);
        ಪ್ರತ್ಯಾಗಮ isEq.isNeqZero();
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
    ಶೂನ್ಯ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (!pinp->exprp()) ಅನುವರ್ತ;
            ಯದಿ (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ಅನುವರ್ತ;
            ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fwdRefp = VN_CAST(pinp->exprp(), ಗಂಟು_Var_Ref);
            ಯದಿ (!fwdRefp) ಅನುವರ್ತ;
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ಧ್ರುವ newIrefp = ನೂತನ AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            ಯದಿ (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ));
            }
            pinp->exprp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    ಶೂನ್ಯ name_By_Pin_ಮೌಲ್ಯ(AstPin* pinp, Ast_ಗಂಟು_Module* srcModp, AstVar* modvarp, string& longnamer,
                        ತಾರ್ಕಿಕ& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(pinp->exprp());
        any_overridesr = ಸತ್ಯ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    ಶೂನ್ಯ cellPinCleanup(Ast_ಗಂಟು* nodep, AstPin* pinp, AstPin* paramsp, Ast_ಗಂಟು_Module* srcModp,
                        string& longnamer, ತಾರ್ಕಿಕ& any_overridesr,
                        std::unordered_set<ಧ್ರುವ AstPin*>& overridingTypePinsr) {
        ಯದಿ (!pinp->exprp()) ಪ್ರತ್ಯಾಗಮ;  // No-connect
        ಯದಿ (AstVar* ಧ್ರುವ modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            ಯದಿ (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_ಮೌಲ್ಯ(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } ಅನ್ಯಥಾ {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                ಯದಿ (is_Aggregate_Param_ಮೌಲ್ಯ(pinp->exprp())) {
                    name_By_Pin_ಮೌಲ್ಯ(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    ಪ್ರತ್ಯಾಗಮ;
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
                ಯದಿ (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    ಧ್ರುವ ತಾರ್ಕಿಕ dependent = modvarp->valuep()->exists(
                        [](Ast_ಗಂಟು* np) { ಪ್ರತ್ಯಾಗಮ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    ಯದಿ (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                ಯದಿ (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ಧ್ರುವ exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ಧ್ರುವ origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                ಯದಿ (exprp && !exprp->ಅಂಕೆ().isDouble() && !exprp->ಅಂಕೆ().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    ತಾರ್ಕಿಕ cloneVarpUnresolved = ಅಸತ್ಯ;
                    ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_ವೃಕ್ಷ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        ಯದಿ (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                        cloneVarp->childDTypep(origDTypep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr ಪೂರ್ಣಾಙ್ಕ maxSubstIters = 1000;
                        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ it = 0; it < maxSubstIters; ++it) {
                            ತಾರ್ಕಿಕ any = ಅಸತ್ಯ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ಧ್ರುವ targetp = varrefp->varp();
                                Ast_ಗಂಟು* replacep = nullptr;
                                ಪ್ರತಿ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ಯದಿ (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        ಯದಿ (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                                        }
                                        ಭಙ್ಗ;
                                    }
                                }
                                ಯದಿ (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ಧ್ರುವ patp = VN_CAST(replacep, Pattern);
                                ಯದಿ (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                                }
                                ಯದಿ (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ವೃಕ್ಷ(), varrefp);
                                    any = ಸತ್ಯ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_ಗಂಟು_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ಧ್ರುವ ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                ಯದಿ (!ptdp) ಪ್ರತ್ಯಾಗಮ;
                                AstPin* overridePinp = nullptr;
                                ಪ್ರತಿ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ಯದಿ (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        ಭಙ್ಗ;
                                    }
                                }
                                Ast_ಗಂಟು_DType* ಧ್ರುವ substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ಗಂಟು_DType)
                                                   : ptdp->subDTypep();
                                ಯದಿ (substp) toReplace.emplace_back(refp, substp);
                            });
                            ಪ್ರತಿ (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ಧ್ರುವ refp = it->first;
                                refp->replaceWith(it->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                                VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                                any = ಸತ್ಯ;
                            }
                            ಯದಿ (!any) ಭಙ್ಗ;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ಗಂಟು* np) {
                            ಯದಿ (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = ಸತ್ಯ;
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                            ಯದಿ (AstEnumItemRef* ಧ್ರುವ itemRefp = VN_CAST(np, EnumItemRef)) {
                                ಯದಿ (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = ಸತ್ಯ;
                                }
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                            AstRefDType* ಧ್ರುವ refp = VN_CAST(np, RefDType);
                            ಯದಿ (!refp) ಪ್ರತ್ಯಾಗಮ;
                            ಯದಿ (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = ಸತ್ಯ;
                            }
                            // A type declared in the template must not be widthed there
                            ಯದಿ (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = ಸತ್ಯ;
                            }
                        });
                    }
                    ಯದಿ (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        ಯದಿ (AstConst* ಧ್ರುವ widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            ಯದಿ (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ವೃಕ್ಷ(), cloneVarp);
                }
                AstConst* ಧ್ರುವ namingExprp = normedNamep ? normedNamep : exprp;
                ಯದಿ (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ಗಂಟು* ಧ್ರುವ pinExprp = pinp->exprp();
                    pinExprp->replaceWith(ನೂತನ AstConst{pinp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ವೃಕ್ಷ(), pinExprp);
                } ಅನ್ಯಥಾ ಯದಿ (origp
                           && (exprp->same_ವೃಕ್ಷ(origp)
                               || (exprp->ಅಂಕೆ().width() == origp->ಅಂಕೆ().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } ಅನ್ಯಥಾ ಯದಿ (namingExprp->ಅಂಕೆ().isDouble() || namingExprp->ಅಂಕೆ().isString()
                           || namingExprp->ಅಂಕೆ().isFourState()
                           || namingExprp->ಅಂಕೆ().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(namingExprp));
                    any_overridesr = ಸತ್ಯ;
                } ಅನ್ಯಥಾ {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->ಅಂಕೆ().ascii(ಅಸತ್ಯ));
                    any_overridesr = ಸತ್ಯ;
                }
                ಯದಿ (normedNamep) VL_DO_DANGLING(normedNamep->delete_ವೃಕ್ಷ(), normedNamep);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ಗಂಟು_DType* rawTypep = VN_CAST(pinp->exprp(), ಗಂಟು_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            ತಾರ್ಕಿಕ skipWidthForTemplateStruct = ಅಸತ್ಯ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ಗಂಟು_DType* ಧ್ರುವ resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                ಯದಿ (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ಗಂಟು_Module* ಧ್ರುವ ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    ಯದಿ (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = ಸತ್ಯ;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                ಯದಿ (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_ಗಂಟು_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ಧ್ರುವ Ast_ಗಂಟು_DType* origp = modvarp->skipRefToNonRefp();
            ಯದಿ (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ಅನ್ಯಥಾ ಯದಿ (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } ಅನ್ಯಥಾ {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                ಯದಿ (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                ಯದಿ (!modvarp->fwdType().is_ಗಂಟು_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                ಯದಿ (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } ಅನ್ಯಥಾ {
                    ಯದಿ (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ಗಂಟು_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        ಯದಿ (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } ಅನ್ಯಥಾ {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    ಶೂನ್ಯ cellInterfaceCleanup(AstPin* pinsp, Ast_ಗಂಟು_Module* srcModp, string& longnamer,
                              ತಾರ್ಕಿಕ& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        ಪ್ರತಿ (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಧ್ರುವ AstVar* ಧ್ರುವ modvarp = pinp->modVarp();
            ಯದಿ (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ಅನುವರ್ತ;
            }
            ಯದಿ (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ಧ್ರುವ portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ exprp = pinp->exprp();
                ಯದಿ (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ಅನುವರ್ತ;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrp = VN_CAST(exprp, ಗಂಟು_Var_Ref)) {
                    ಯದಿ (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                ಯದಿ (!pinIrefp && exprp) {
                    ಧ್ರುವ Ast_ಗಂಟು* basep = exprp;
                    ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ selp = VN_CAST(basep, ಗಂಟು_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vrp = VN_CAST(basep, VarRef)) {
                        ಯದಿ (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                ಯದಿ (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } ಅನ್ಯಥಾ ಯದಿ (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } ಅನ್ಯಥಾ {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    ಯದಿ (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(pinIrefp));
                        any_overridesr = ಸತ್ಯ;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        ಯದಿ (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ಗಂಟು::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ಗಂಟು::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ storeOriginalParams(Ast_ಗಂಟು_Module* ಧ್ರುವ classp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ಗಂಟು* originalParamp = nullptr;
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isGParam()) originalParamp = varp->clonep();
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ಯದಿ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ಯದಿ (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    ಶೂನ್ಯ genericInterfaceVarSetup(ಧ್ರುವ AstPin* ಧ್ರುವ paramsp, ಧ್ರುವ AstPin* ಧ್ರುವ pinsp) {
        std::unordered_map<string, ಧ್ರುವ AstPin*> paramspMap;
        ಪ್ರತಿ (ಧ್ರುವ AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ಸೇರಿಸು({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        ಯದಿ (paramspMap.empty()) ಪ್ರತ್ಯಾಗಮ;

        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstPin* ಧ್ರುವ pinp = VN_CAST(nodep, Pin)) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = pinp->modVarp()) {
                    Ast_ಗಂಟು_DType* dtypep = varp->childDTypep()->elemDTypep();
                    ಯದಿ (AstIfaceGenericDType* ಧ್ರುವ ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ಧ್ರುವ auto iter = paramspMap.find(varp->name());
                        ಯದಿ (iter == paramspMap.end()) ಅನುವರ್ತ;
                        Ast_ಗಂಟು* ಧ್ರುವ backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ಧ್ರುವ AstPin* ಧ್ರುವ paramp = iter->second;
                        paramspMap.erase(iter);
                        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ಧ್ರುವ newIfacerefp = ನೂತನ AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        ಯದಿ (auto* ಧ್ರುವ arrDtp = VN_CAST(backp, ಗಂಟು_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } ಅನ್ಯಥಾ ಯದಿ (auto* ಧ್ರುವ arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } ಅನ್ಯಥಾ {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ತಳ್ಳು_Deletep(ifaceGDTypep), ifaceGDTypep);
                        ಯದಿ (paramspMap.empty()) ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    ಶೂನ್ಯ resolveDefaultParams(Ast_ಗಂಟು* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        ಯದಿ (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            ಯದಿ (!classp) ಪ್ರತ್ಯಾಗಮ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } ಅನ್ಯಥಾ ಯದಿ (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                ಯದಿ (!paramTypep->isGParam()) ಅನುವರ್ತ;
                m_paramIndex.emplace(paramTypep, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_ಗಂಟು_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        ಪ್ರತಿ (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* ಧ್ರುವ paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ಧ್ರುವ reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            ಯದಿ (!reqDtp) ಅನುವರ್ತ;

            // If default is a reference to another param type, record dependency
            ಯದಿ (AstRefDType* ಧ್ರುವ refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                ಯದಿ (AstParamTypeDType* ಧ್ರುವ sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    ಯದಿ (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    ಅನುವರ್ತ;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(reqDtp->lhsp(), ಗಂಟು_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ಧ್ರುವ typep = pinp->modPTypep();
            ಧ್ರುವ auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            ಯದಿ (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } ಅನ್ಯಥಾ {
                pinsByIndex.ತಳ್ಳು_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        ಪ್ರತಿ (ತಾರ್ಕಿಕ resolvedSome = ಸತ್ಯ; resolvedSome;) {
            resolvedSome = ಅಸತ್ಯ;
            ಪ್ರತಿ (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                ಯದಿ (pinsByIndex[paramIdx]) ಅನುವರ್ತ;
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                ಯದಿ (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                ಯದಿ (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = defaultTypeNodes[paramIdx];
                    newPinp = ನೂತನ AstPin{dtypep->fileline(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                }

                ಯದಿ (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(ಸತ್ಯ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    ಯದಿ (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } ಅನ್ಯಥಾ ಯದಿ (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    ಯದಿ (!paramsp) paramsp = newPinp;
                    resolvedSome = ಸತ್ಯ;
                }
            }
        }
    }

    Ast_ಗಂಟು_Module* ಗಂಟು_Deparam_Common(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, ತಾರ್ಕಿಕ any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        ಯದಿ (debug() >= 9 && paramsp) paramsp->dump_ವೃಕ್ಷ_And_Next(cout, "-  cellparams: ");

        ಯದಿ (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } ಅನ್ಯಥಾ {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ಧ್ರುವ AstPin*> overridingTypePins;
            ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (!pinp->exprp()) ಅನುವರ್ತ;  // An empty override keeps the default
                ಯದಿ (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ಅನುವರ್ತ;
                }
                AstVar* ಧ್ರುವ modvarp = pinp->modVarp();
                ಯದಿ (!modvarp || !modvarp->isGParam()) ಅನುವರ್ತ;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ಧ್ರುವ AstPattern* ಧ್ರುವ patternp = VN_CAST(pinp->exprp(), Pattern);
                ಯದಿ (patternp && !patternp->childDTypep()) ಅನುವರ್ತ;
                ಯದಿ (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_ಮೌಲ್ಯ(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (AstParamTypeDType* ಧ್ರುವ modptp = pinp->modPTypep()) {
                    ಯದಿ (!overridingTypePins.ಎಣಿಕೆ(pinp)) ಅನುವರ್ತ;
                    Ast_ಗಂಟು_DType* ಧ್ರುವ typep = VN_AS(pinp->exprp(), ಗಂಟು_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_ಮೌಲ್ಯ_ಸಂಖ್ಯೆ(typep->skipRefToNonRefp());
                    any_overrides = ಸತ್ಯ;
                    ಅನುವರ್ತ;
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
        ತಾರ್ಕಿಕ defaultsResolved = ಅಸತ್ಯ;
        ಯದಿ (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            ಪ್ರತಿ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ಯದಿ (pinp->modPTypep()) {
                    any_overrides = ಸತ್ಯ;
                    defaultsResolved = ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
        }
        ಯದಿ (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = ಸತ್ಯ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ಗಂಟು_Module* newModp = nullptr;
        ಯದಿ (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            ಯದಿ (!paramedModp) ಪ್ರತ್ಯಾಗಮ nullptr;  // Unsupported, already reported
            paramedModp->dead(ಅಸತ್ಯ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } ಅನ್ಯಥಾ ಯದಿ (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            ಯದಿ (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ಗಂಟು_Module* ಗಂಟು_Copyp = srcModp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ಗಂಟು_Copyp);
                m_deleter.ತಳ್ಳು_Deletep(ಗಂಟು_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ಗಂಟು_Copyp);
                storeOriginalParams(ಗಂಟು_Copyp);
            }
            newModp = srcModp;
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ಧ್ರುವ ModInfo* ಧ್ರುವ modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            ಯದಿ (!modInfop) ಪ್ರತ್ಯಾಗಮ nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ಧ್ರುವ ತಾರ್ಕಿಕ cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ಧ್ರುವ newClassp = VN_CAST(newModp, Class);
        ಯದಿ (newClassp && !newClassp->user2()) m_specializedClassps.ತಳ್ಳು_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        ಯದಿ (defaultsResolved) srcModp->user4p(newModp);

        ಪ್ರತಿ (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                ಯದಿ (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        ಯದಿ (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ(), paramsp);
        ಪ್ರತ್ಯಾಗಮ newModp;
    }

    Ast_ಗಂಟು_Module* cellDeparam(AstCell* nodep, Ast_ಗಂಟು_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ಗಂಟು_Module* ಧ್ರುವ newModp = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        ಯದಿ (!newModp) ಪ್ರತ್ಯಾಗಮ nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(ಅಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ newModp;
    }
    Ast_ಗಂಟು_Module* ifaceRefDeparam(AstIfaceRefDType* ಧ್ರುವ nodep, Ast_ಗಂಟು_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        ಯದಿ (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ಧ್ರುವ string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ಧ್ರುವ string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ಧ್ರುವ size_t ownerPos = ownerBaseName.find("__");
            ಯದಿ (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ಧ್ರುವ size_t srcPos = srcBaseName.find("__");
            ಯದಿ (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            ಯದಿ (ownerBaseName == srcBaseName) {
                ತಾರ್ಕಿಕ allOwnParams = ಸತ್ಯ;
                ಪ್ರತಿ (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    ಯದಿ (AstRefDType* ಧ್ರುವ refp = VN_CAST(pinp->exprp(), RefDType)) {
                        ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ಗಂಟು_Module* ಧ್ರುವ ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            ಯದಿ (ptdOwnerp != m_modp) allOwnParams = ಅಸತ್ಯ;
                        } ಅನ್ಯಥಾ {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } ಅನ್ಯಥಾ {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                ಯದಿ (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    ಯದಿ (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
                    ಪ್ರತ್ಯಾಗಮ ownerIfacep;
                }
            }
        }

        Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ಅಸತ್ಯ);
        ಯದಿ (!newModp) ಪ್ರತ್ಯಾಗಮ nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        ಪ್ರತ್ಯಾಗಮ newModp;
    }
    Ast_ಗಂಟು_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ಗಂಟು_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ಅಸತ್ಯ);
        ಯದಿ (!newModp) ಪ್ರತ್ಯಾಗಮ nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ಧ್ರುವ refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ಧ್ರುವ newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        ಯದಿ (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            ಯದಿ (AstTypedef* ಧ್ರುವ typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        ಪ್ರತ್ಯಾಗಮ newModp;
    }
    Ast_ಗಂಟು_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ಗಂಟು_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
            = ಗಂಟು_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, ಅಸತ್ಯ);
        ಯದಿ (!newModp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstClass* ಧ್ರುವ newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        ಪ್ರತ್ಯಾಗಮ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    ವರ್ಗ DeferredResolverVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ಧ್ರುವ Ast_ಗಂಟು*> m_reachedDecls;

        ತಾರ್ಕಿಕ firstReach(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ declp) { ಪ್ರತ್ಯಾಗಮ m_reachedDecls.ಸೇರಿಸು(declp).second; }

        ಶೂನ್ಯ iterateParamType(AstParamTypeDType* ಧ್ರುವ paramTypep) {
            ಯದಿ (firstReach(paramTypep)) iterate(paramTypep);
        }
        ಶೂನ್ಯ iterateTypedef(AstTypedef* ಧ್ರುವ tdefp) {
            ಯದಿ (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
        ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ಗಂಟು* ಧ್ರುವ targetp = nodep->classOrPackageNodep();
            ಯದಿ (AstTypedef* ಧ್ರುವ tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        ಶೂನ್ಯ visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        ಶೂನ್ಯ visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ಧ್ರುವ tdefp = nodep->typedefp();
            ಯದಿ (tdefp) {
                iterateTypedef(tdefp);
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        ಶೂನ್ಯ visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ಧ್ರುವ varp = nodep->varp();
            ಯದಿ (firstReach(varp)) {
                iterate(varp->subDTypep());
                ಧ್ರುವ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                ಯದಿ (varp->varType() == VVarType::LPARAM && deferredVarps.ಎಣಿಕೆ(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    ಸಾರ್ವಜನಿಕ:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ಗಂಟು* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

ಸಾರ್ವಜನಿಕ:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    ಶೂನ್ಯ retargetIfaceRefs(Ast_ಗಂಟು_Module* parentModp, ಧ್ರುವ string& cellName) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        ಯದಿ (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) ಪ್ರತ್ಯಾಗಮ;
        parentModp->foreach([&](AstRefDType* refp) {
            ಧ್ರುವ VIfaceCaptureTag* ಧ್ರುವ tagp = V3LinkDotIfaceCapture::captureTag(refp);
            ಯದಿ (!tagp || tagp->m_cellPath != cellName) ಪ್ರತ್ಯಾಗಮ;
            // Only retarget REFDTYPEs owned by parentModp.
            ಯದಿ (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    ಶೂನ್ಯ resolveDeferredDotsReachableFrom(Ast_ಗಂಟು* rootp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ಏತತ್, rootp};
    }

    Ast_ಗಂಟು_Module* ಗಂಟು_Deparam(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* srcModp, Ast_ಗಂಟು_Module* modp,
                               ಧ್ರುವ string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ಗಂಟು_Name = nodep->name();
        ಯದಿ (AstIfaceRefDType* ಧ್ರುವ ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            ಯದಿ (ಗಂಟು_Name.empty()) ಗಂಟು_Name = ifaceRefp->cellName();
        }
        ಧ್ರುವ string instanceName
            = ಗಂಟು_Name.empty() ? someInstanceName : (someInstanceName + "." + ಗಂಟು_Name);

        ಯದಿ (AstCell* ಧ್ರುವ cellp = VN_CAST(nodep, Cell)) {
            ಪ್ರತಿ (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ಧ್ರುವ nextp = VN_AS(pinp->nextp(), Pin);
                ಯದಿ (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_ವೃಕ್ಷ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ಗಂಟು_Module* newModp = nullptr;
        ಯದಿ (AstCell* ಧ್ರುವ cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } ಅನ್ಯಥಾ ಯದಿ (AstIfaceRefDType* ಧ್ರುವ ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } ಅನ್ಯಥಾ ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ಅನ್ಯಥಾ ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        ಯದಿ (!newModp) ಪ್ರತ್ಯಾಗಮ nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ಗಂಟು::ಗಂಟು_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        ಪ್ರತ್ಯಾಗಮ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        ಪ್ರತ್ಯಾಗಮ taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            m_allModuleNames.ಸೇರಿಸು(modp->name());
        }
    }
    ~ParamProcessor() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

ವರ್ಗ ClassRefUnlinkerVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

ಸಾರ್ವಜನಿಕ:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        ಯದಿ (nodep->paramsp()) {
            ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                ಯದಿ (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ಧ್ರುವ dotp = VN_CAST(nodep->backp(), Dot);
                    ಯದಿ (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(ತಳ್ಳು_Deletep(dotp), dotp, nodep);
                    } ಅನ್ಯಥಾ {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {}  // don't iterate inside classes
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

ವರ್ಗ ParamClassRefDTypeRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ಧ್ರುವ AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ಗಂಟು_Module* m_ownerModp = nullptr;
    Ast_ಗಂಟು_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    ಶೂನ್ಯ ensureOwnerMap() {
        ಯದಿ (m_ownerMapForp == m_ownerModp) ಪ್ರತ್ಯಾಗಮ;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        ಯದಿ (!m_ownerModp) ಪ್ರತ್ಯಾಗಮ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        ಯದಿ (AstClass* ಧ್ರುವ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ಯದಿ (ownerClassp->hasGParam()) ಪ್ರತ್ಯಾಗಮ;
        }
        ಪ್ರತಿ (Ast_ಗಂಟು* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ಧ್ರುವ tdp = VN_CAST(sp, Typedef);
            ಯದಿ (!tdp) ಅನುವರ್ತ;
            AstClassRefDType* ಧ್ರುವ crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            ಯದಿ (!crdtp || !crdtp->classp()) ಅನುವರ್ತ;
            AstClass* ಧ್ರುವ targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            ಯದಿ (targetp->hasGParam()) ಅನುವರ್ತ;
            ಧ್ರುವ std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ಧ್ರುವ auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            ಯದಿ (!pair.second && pair.first->second != targetp) m_ambiguous.ಸೇರಿಸು(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ಸ್ಥಿರ string classOrigName(ಧ್ರುವ AstClass* classp) {
        ಪ್ರತ್ಯಾಗಮ classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ಸ್ಥಿರ AstTypedef* findTypedefWithBases(AstClass* classp, ಧ್ರುವ string& name) {
        ಪ್ರತಿ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ಯದಿ (AstTypedef* ಧ್ರುವ tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                ಯದಿ (tdp->subDTypep()) ಪ್ರತ್ಯಾಗಮ tdp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    ಶೂನ್ಯ retargetRefDType(AstRefDType* refp) {
        ಯದಿ (!m_ownerModp) ಪ್ರತ್ಯಾಗಮ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        ಯದಿ (AstClass* ಧ್ರುವ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ಯದಿ (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ಧ್ರುವ AstClass* refClassp = nullptr;
                ಯದಿ (AstTypedef* ಧ್ರುವ tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassOrPackageRef* ಧ್ರುವ classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                ಯದಿ (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ಧ್ರುವ selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    ಯದಿ (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        AstTypedef* ಧ್ರುವ oldTdp = refp->typedefp();
        ಯದಿ (!oldTdp) ಪ್ರತ್ಯಾಗಮ;
        AstClass* ಧ್ರುವ oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        ಯದಿ (!oldOwnerp) ಪ್ರತ್ಯಾಗಮ;

        ensureOwnerMap();
        ಯದಿ (m_origNameToClone.empty()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        ಯದಿ (m_ambiguous.ಎಣಿಕೆ(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ auto it = m_origNameToClone.find(origName);
        ಯದಿ (it == m_origNameToClone.end()) ಪ್ರತ್ಯಾಗಮ;
        AstClass* ಧ್ರುವ correctClassp = it->second;
        ಯದಿ (correctClassp == oldOwnerp) ಪ್ರತ್ಯಾಗಮ;
        AstTypedef* ಧ್ರುವ newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        ಯದಿ (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        ಯದಿ (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

ಸಾರ್ವಜನಿಕ:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        ಯದಿ (AstParamTypeDType* ಧ್ರುವ paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ಧ್ರುವ auto it = m_paramTypeClassMap.find(paramtypep);
            ಯದಿ (it != m_paramTypeClassMap.end()) {
                AstClass* ಧ್ರುವ origClassp = it->second;
                ಯದಿ (origClassp->hasGParam() && !origClassp->user3p()) {
                    ಯದಿ (AstClass* ಧ್ರುವ parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ಧ್ರುವ string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ಯದಿ (AstParamTypeDType* ಧ್ರುವ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                ಯದಿ (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    ಭಙ್ಗ;
                                }
                            }
                        }
                        ಯದಿ (newParamTypep) {
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

ವರ್ಗ ParamState final {
ಸಾರ್ವಜನಿಕ:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<ತಾರ್ಕಿಕ, ಪೂರ್ಣಾಙ್ಕ>;
    std::multimap<WQKey, Ast_ಗಂಟು_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ಗಂಟು_Module*, std::unordered_set<Ast_ಗಂಟು_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

ವರ್ಗ ParamVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    ತಾರ್ಕಿಕ m_iterateModule = ಅಸತ್ಯ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<ತಾರ್ಕಿಕ, Ast_ಗಂಟು*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ಧ್ರುವ AstRefDType*, ತಾರ್ಕಿಕ>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    ತಾರ್ಕಿಕ m_modIfaceRefsDone = ಅಸತ್ಯ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    ತಾರ್ಕಿಕ m_inGenerateCond = ಅಸತ್ಯ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    ಶೂನ್ಯ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ಗಂಟು_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = ಸತ್ಯ;

        // Visit all cells under module, recursively
        ಯಾವತ್ (ಸತ್ಯ) {
            // Classes specialized since the last pass still need their bodies elaborated
            ಪ್ರತಿ (AstClass* ಧ್ರುವ classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{ಸತ್ಯ, classp->level()}, classp);
            }
            ಯದಿ (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            ಯದಿ (workQueue.empty()) ಭಙ್ಗ;

            ಧ್ರುವ auto itm = workQueue.cbegin();
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            ಯದಿ (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                ಯದಿ (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = ಅಸತ್ಯ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            ಯಾವತ್ (!m_cellps.empty()) {
                ಧ್ರುವ auto itim = m_cellps.cbegin();
                Ast_ಗಂಟು* ಧ್ರುವ cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ಧ್ರುವ auto genHierIt = m_genHierNames.find(cellp);
                ಯದಿ (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_ಗಂಟು_Module* srcModp = nullptr;
                ಯದಿ (ಧ್ರುವ AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    ಯದಿ (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } ಅನ್ಯಥಾ {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                ಯದಿ (!srcModp) ಅನುವರ್ತ;

                // Update path
                ಧ್ರುವ string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
                    = m_processor.ಗಂಟು_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    ಯದಿ (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        ಯದಿ (V3LinkDotIfaceCapture::enabled()) {
                            ಯದಿ (ಧ್ರುವ AstCell* ಧ್ರುವ modCellp = VN_CAST(cellp, Cell)) {
                                ಯದಿ (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ಸೇರಿಸು(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    ಯದಿ (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = ಅಸತ್ಯ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ಸ್ಥಿರ string getRefBaseName(ಧ್ರುವ AstVarXRef* refp) {
        ಧ್ರುವ string dotted = refp->dotted();
        ಯದಿ (dotted.empty()) ಪ್ರತ್ಯಾಗಮ "";
        ಪ್ರತ್ಯಾಗಮ dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    ಶೂನ್ಯ logTemplateLeakRefs(Ast_ಗಂಟು_Module* parentModp, Ast_ಗಂಟು_Module* templateModp,
                             ಧ್ರುವ ವರ್ಣ* stage, Ast_ಗಂಟು* contextp) {
        ಯದಿ (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) ಪ್ರತ್ಯಾಗಮ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        ಪೂರ್ಣಾಙ್ಕ leak_ಎಣಿಕೆ = 0;
        ಧ್ರುವ auto ancestryOf = [](ಧ್ರುವ Ast_ಗಂಟು* nodep) {
            string ancestry;
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
                ಯದಿ (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                ಯದಿ (VN_IS(curp, ಗಂಟು_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    ಭಙ್ಗ;
                }
            }
            ಪ್ರತ್ಯಾಗಮ ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            ಯದಿ (refp->typedefp()) {
                Ast_ಗಂಟು_Module* ಧ್ರುವ tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                ಯದಿ (tdOwnerp == templateModp) {
                    ++leak_ಎಣಿಕೆ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ಗಂಟು::ಗಂಟು_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            ಯದಿ (refp->refDTypep()) {
                Ast_ಗಂಟು_Module* ಧ್ರುವ rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                ಯದಿ (rdOwnerp == templateModp) {
                    ++leak_ಎಣಿಕೆ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ಗಂಟು::ಗಂಟು_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            ಯದಿ (!varrefp->varp()) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_Module* ಧ್ರುವ varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            ಯದಿ (varOwnerp != templateModp) ಪ್ರತ್ಯಾಗಮ;
            ++leak_ಎಣಿಕೆ;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ಗಂಟು::ಗಂಟು_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        ಯದಿ (leak_ಎಣಿಕೆ > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ಎಣಿಕೆ);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    ಶೂನ್ಯ checkParamNotHierRecurse(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ underTypeQuery = ಅಸತ್ಯ) {
        ಪ್ರತಿ (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ಧ್ರುವ AstAttrOf* ಧ್ರುವ attrp = VN_CAST(nodep, AttrOf);
            ಧ್ರುವ ತಾರ್ಕಿಕ childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            ಯದಿ (ಧ್ರುವ AstVarXRef* ಧ್ರುವ refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                ತಾರ್ಕಿಕ isIfaceRef = ಅಸತ್ಯ;
                ಯದಿ (refp->varp() && refp->varp()->isIfaceParam()) {
                    ಧ್ರುವ string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.ಎಣಿಕೆ(refname) || m_ifaceInstCells.ಎಣಿಕೆ(refname));
                }
                ಯದಿ (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ refp = VN_CAST(nodep, ಗಂಟು_FTask_Ref)) {
                ಯದಿ (refp->dotted() != "") {
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
    ಶೂನ್ಯ specializeNestedIfaceCells(Ast_ಗಂಟು_Module* ifaceModp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ಧ್ರುವ nestedCellp = VN_CAST(stmtp, Cell);
            ಯದಿ (!nestedCellp) ಅನುವರ್ತ;
            ಯದಿ (!VN_IS(nestedCellp->modp(), Iface)) ಅನುವರ್ತ;
            ಯದಿ (!nestedCellp->paramsp()) ಅನುವರ್ತ;

            Ast_ಗಂಟು_Module* ಧ್ರುವ nestedSrcModp = nestedCellp->modp();
            ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ nestedNewModp = m_processor.ಗಂಟು_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                ಯದಿ (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    ಪ್ರತಿ (Ast_ಗಂಟು* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(sp, Var)) {
                            ಯದಿ (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    ಯದಿ (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    ತಾರ್ಕಿಕ cellParamsReferenceIfacePorts(AstCell* cellp) {
        ಯದಿ (!cellp->paramsp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (AstPin* ಧ್ರುವ pinp = VN_CAST(nodep, Pin)) {
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ exprp = pinp->exprp()) {
                    ಯದಿ (ಧ್ರುವ AstVarXRef* ಧ್ರುವ refp = VN_CAST(exprp, VarXRef)) {
                        ಧ್ರುವ string refname = getRefBaseName(refp);
                        ಯದಿ (!refname.empty()
                            && (m_ifacePortNames.ಎಣಿಕೆ(refname)
                                || m_ifaceInstCells.ಎಣಿಕೆ(refname)))
                            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    ಶೂನ್ಯ expandIfaceArrayPortDimensions(AstVar* portp, Ast_ಗಂಟು_DType* dtypep,
                                        ಧ್ರುವ std::string& suffix) {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        ಯದಿ (!arrp) {
            AstVar* ಧ್ರುವ varp = portp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ oldp = varp->childDTypep()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            portp->addNextHere(varp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ n : arrp->declRange().seq_ಬಲ_To_ಎಡ()) {
            ಧ್ರುವ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    ಶೂನ್ಯ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ಧ್ರುವ AstRange* rangep, ಧ್ರುವ std::string& suffix, ಪೂರ್ಣಾಙ್ಕ idx) {
        // Base case: insert the element when no dimensions left
        ಯದಿ (!rangep) {
            AstCell* ಧ್ರುವ elemp = arrayedCellp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            ಯದಿ (ifaceVarp) {
                AstIfaceRefDType* ಧ್ರುವ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ಧ್ರುವ varp = ifaceVarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ಧ್ರುವ AstRange* ಧ್ರುವ subRangep = VN_AS(rangep->nextp(), Range);
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ n : rangep->seq_ಬಲ_To_ಎಡ()) {
            ಧ್ರುವ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    ಶೂನ್ಯ visitCellOrClassRef(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        ಯದಿ (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ಧ್ರುವ cellp = VN_CAST(nodep, Cell);
            ಯದಿ (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ಗಂಟು_Module* ಧ್ರುವ srcModp = cellp->modp();
                Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
                    = m_processor.ಗಂಟು_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                ಯದಿ (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                ಯದಿ (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        ಯದಿ (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->recursiveClone()) nodep->dead(ಸತ್ಯ);  // Fake, made for recursive elimination
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            ಯದಿ (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ತಳ್ಳು_back(classp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Start traversal at root-like things
        ಯದಿ (nodep->is_ತುದಿ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    ತಾರ್ಕಿಕ isCircularType(ಧ್ರುವ AstRefDType* nodep) {
        ಧ್ರುವ auto iter = m_isCircular.emplace(nodep, ಸತ್ಯ);
        ಯದಿ (!iter.second) ಪ್ರತ್ಯಾಗಮ iter.first->second;
        ಯದಿ (ಧ್ರುವ AstRefDType* ಧ್ರುವ subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ಧ್ರುವ ತಾರ್ಕಿಕ ret = isCircularType(subDTypep);
            iter.first->second = ret;
            ಪ್ರತ್ಯಾಗಮ ret;
        }
        iter.first->second = ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        ಯದಿ (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        ಯದಿ (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->typedefp() && nodep->subDTypep()
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
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        ಯದಿ (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            ಪ್ರತಿ (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ಧ್ರುವ ifaceVarp = [&]() -> AstVar* {
                ಯದಿ (!VN_IS(nodep->modp(), Iface)) ಪ್ರತ್ಯಾಗಮ nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ಧ್ರುವ varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                ಪ್ರತ್ಯಾಗಮ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            ಯದಿ (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(ಸತ್ಯ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ಧ್ರುವ firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ಧ್ರುವ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            ಪ್ರತ್ಯಾಗಮ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        ಯದಿ (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        ಯದಿ (nodep->ifacep()) visitCellOrClassRef(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        ಯದಿ (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, ಅಸತ್ಯ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ಸ್ಥಿರ ಶೂನ್ಯ gatherVars(Ast_ಗಂಟು* stmtsp, ತಾರ್ಕಿಕ (*matchp)(ಧ್ರುವ AstVar*), VarsByName& varsr) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                ಯದಿ (matchp(varp)) varsr.emplace(varp->name(), varp);
            } ಅನ್ಯಥಾ ಯದಿ (AstGenBlock* ಧ್ರುವ genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ಧ್ರುವ VarsByName& modIfaceRefs() {
        ಯದಿ (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ಧ್ರುವ AstVar* varp) { ಪ್ರತ್ಯಾಗಮ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ m_modIfaceRefs;
    }
    ಧ್ರುವ VarsByName& ifaceParams(Ast_ಗಂಟು_Module* ifacep) {
        ಧ್ರುವ auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        ಯದಿ (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ಧ್ರುವ AstVar* varp) { ಪ್ರತ್ಯಾಗಮ varp->isParam(); },
                pair.first->second);
        }
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ಗಂಟು* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ಧ್ರುವ lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ಧ್ರುವ rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        ಯದಿ (!lhsp || !rhsp) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        ಯದಿ (VL_UNCOVERABLE(!m_modp)) ಪ್ರತ್ಯಾಗಮ nullptr;

        ಧ್ರುವ auto& refVars = modIfaceRefs();
        ಧ್ರುವ auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        ಯದಿ (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVar* ಧ್ರುವ ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ಧ್ರುವ ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        ಯದಿ (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) ಪ್ರತ್ಯಾಗಮ nullptr;

        ಧ್ರುವ auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ಧ್ರುವ auto targetVarIt = ifaceVars.find(rhsp->name());
        ಯದಿ (targetVarIt == ifaceVars.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVar* ಧ್ರುವ targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        ಯದಿ (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        ಯದಿ (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            ಪ್ರತ್ಯಾಗಮ nullptr;  // LCOV_EXCL_LINE
        }
        ಪ್ರತ್ಯಾಗಮ targetVarp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ resolveGenericIfaceDotsIn(Ast_ಗಂಟು* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ತಳ್ಳು_back(dotp); });
        ಪ್ರತಿ (AstDot* ಧ್ರುವ dotp : dotps) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (nodep->user2SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        // Interface array port: split it into its elements
        ಯದಿ (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(ಸತ್ಯ);
        }
        // Build cache of interface port names as we encounter them
        ಯದಿ (nodep->isIfaceRef()) { m_ifacePortNames.ಸೇರಿಸು(nodep->name()); }
        iterateChildren(nodep);
        ಯದಿ (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            ಯದಿ (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                ತಾರ್ಕಿಕ hasUnresolvedLparamXRef = ಅಸತ್ಯ;
                nodep->valuep()->foreach([&](ಧ್ರುವ AstVarXRef* xrefp) {
                    ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = xrefp->varp()) {
                        ಯದಿ (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = ಸತ್ಯ;
                        }
                    }
                });
                ಯದಿ (hasUnresolvedLparamXRef) ಪ್ರತ್ಯಾಗಮ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ಧ್ರುವ ತಾರ್ಕಿಕ hasDot = nodep->valuep()->exists([](AstDot*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
                ತಾರ್ಕಿಕ refsDeferred = ಅಸತ್ಯ;
                ಯದಿ (!hasDot) {
                    ಧ್ರುವ auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ಧ್ರುವ AstVarRef* refp) {
                        ಯದಿ (refsDeferred) ಪ್ರತ್ಯಾಗಮ;
                        AstVar* ಧ್ರುವ refVarp = refp->varp();
                        ಯದಿ (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ಎಣಿಕೆ(refVarp)) {
                            refsDeferred = ಸತ್ಯ;
                        }
                    });
                }
                ಯದಿ (hasDot || refsDeferred) {
                    v3Global.rootp()->ತಳ್ಳು_Deferred_Param_Varp(nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        ಯದಿ (nodep->varp()) iterate(nodep->varp());
    }
    ತಾರ್ಕಿಕ ifaceParamReplace(AstVarXRef* nodep, Ast_ಗಂಟು* candp) {
        ಪ್ರತಿ (; candp; candp = candp->nextp()) {
            ಯದಿ (nodep->name() == candp->name()) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    ಯದಿ (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPin* ಧ್ರುವ pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ deparamIfaceCellNow(AstCell* cellp) {
        ಯದಿ (!cellp->paramsp()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!VN_IS(cellp->modp(), Iface)) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_Module* ಧ್ರುವ srcModp = cellp->modp();
        Ast_ಗಂಟು_Module* ಧ್ರುವ newModp
            = m_processor.ಗಂಟು_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        ಯದಿ (newModp && newModp != srcModp) {
            ಯದಿ (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        ಯದಿ (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        ಯದಿ (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Parameters of an interface array are accessed through its first element
        ಯದಿ (nodep->varp() && nodep->varp()->isParam()) {
            ಧ್ರುವ auto it = m_ifaceInstCells.find(nodep->dotted());
            ಯದಿ (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ಧ್ರುವ string dotted = nodep->dotted();
        ಯದಿ (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ಧ್ರುವ Ast_ಗಂಟು* backp = nodep;
            ಯಾವತ್ ((backp = backp->backp())) {
                ಯದಿ (VN_IS(backp, ಗಂಟು_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    ಭಙ್ಗ;
                }
                ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(backp, Var)) {
                    ಯದಿ (!varp->isIfaceRef()) ಅನುವರ್ತ;
                    // Through all dimensions of an array
                    ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    ಯದಿ (!ifacerefp) ಅನುವರ್ತ;
                    // Interfaces passed in on the port map have ifaces
                    ಯದಿ (ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = ifacerefp->ifacep()) {
                        ಯದಿ (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            ಯದಿ (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    ಅನ್ಯಥಾ ಯದಿ (AstCell* ಧ್ರುವ cellp = ifacerefp->cellp()) {
                        ಯದಿ (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            ಯದಿ (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ಧ್ರುವ auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            ಯದಿ (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ಧ್ರುವ cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    ಶೂನ್ಯ visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ಧ್ರುವ AstClass* lhsClassp = nullptr;
        ಧ್ರುವ AstClassOrPackageRef* ಧ್ರುವ classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        ಯದಿ (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ಗಂಟು* rhsDefp = nullptr;
        AstClassOrPackageRef* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        ಯದಿ (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        ಯದಿ (lhsClassp && rhsDefp) {
            m_state.m_dots.ತಳ್ಳು_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } ಅನ್ಯಥಾ {
            iterate(nodep->rhsp());
        }
    }

    ಶೂನ್ಯ visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ಧ್ರುವ varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ taskrefp = VN_CAST(nodep->refp(), ಗಂಟು_FTask_Ref);
        AstEnumItemRef* ಧ್ರುವ enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        ಯದಿ (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } ಅನ್ಯಥಾ ಯದಿ (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } ಅನ್ಯಥಾ ಯದಿ (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterate(nodep->cellrefp());

        ಯದಿ (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } ಅನ್ಯಥಾ ಯದಿ (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } ಅನ್ಯಥಾ {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        ಪ್ರತಿ (Ast_ಗಂಟು *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<ಪೂರ್ಣಾಙ್ಕ> indices;
        ಪ್ರತಿ (Ast_ಗಂಟು* s = nodep->selp(); s; s = s->nextp()) {
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(s, Const);
            ಯದಿ (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ;
            }
            indices.ತಳ್ಳು_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ಧ್ರುವ string viftopSuffix = "__Viftop";
        ಧ್ರುವ string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        ಧ್ರುವ string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        ಯದಿ (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        pos += baseName.length();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ idx : indices) {
            ಧ್ರುವ string replacement = "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(idx) + "__KET__";
            ಯದಿ (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                ಪ್ರತ್ಯಾಗಮ;
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
    ಶೂನ್ಯ iterateGenerateCond(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = ಸತ್ಯ;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    ಶೂನ್ಯ visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->condp(), Const)) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } ಅನ್ಯಥಾ {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Normal edit rules will now recurse the replacement
        } ಅನ್ಯಥಾ {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        ಯದಿ (AstGenFor* ಧ್ರುವ forp = VN_AS(nodep->genforp(), GenFor)) {
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
            ಧ್ರುವ string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } ಅನ್ಯಥಾ {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    ಶೂನ್ಯ visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        ತಾರ್ಕಿಕ hit = ಅಸತ್ಯ;
        Ast_ಗಂಟು* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        ಪ್ರತಿ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ಧ್ರುವ AstConst* ಧ್ರುವ exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        ಪ್ರತಿ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep;) {
                Ast_ಗಂಟು* ಧ್ರುವ nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        ಪ್ರತಿ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ಯದಿ (!itemp->isDefault()) {
                ಪ್ರತಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ ccondp = VN_CAST(ep, Const)) {
                        V3_ಸಂಖ್ಯೆ match{nodep, 1};
                        match.opEq(ccondp->ಅಂಕೆ(), exprp->ಅಂಕೆ());
                        ಯದಿ (!hit && match.isNeqZero()) {
                            hit = ಸತ್ಯ;
                            keepp = itemp->itemsp();
                        }
                    } ಅನ್ಯಥಾ {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        ಪ್ರತಿ (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            ಯದಿ (itemp->isDefault()) {
                ಯದಿ (!hit) {
                    hit = ಸತ್ಯ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        ಯದಿ (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

ವರ್ಗ Param_ತುದಿ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    ಶೂನ್ಯ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        ಪ್ರತಿ (AstDot* ಧ್ರುವ dotp : m_state.m_dots) {
            ಧ್ರುವ AstClassOrPackageRef* ಧ್ರುವ classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ಧ್ರುವ AstClass* ಧ್ರುವ lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ಧ್ರುವ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            ಪ್ರತಿ (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    ಭಙ್ಗ;
                }
            }
        }
    }

    ಶೂನ್ಯ fixLevel(Ast_ಗಂಟು_Module* modp) {
        // Fix up level of module, based on who instantiates it
        ಯದಿ (modp->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Already fixed
        ಯದಿ (m_state.m_parentps[modp].empty()) ಪ್ರತ್ಯಾಗಮ;  // Leave top levels alone
        ಪೂರ್ಣಾಙ್ಕ maxParentLevel = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        ಯದಿ (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    ಶೂನ್ಯ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_ಗಂಟು_Module*> modps;
        ಪ್ರತಿ (Ast_ಗಂಟು_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ಗಂಟು_Module);
            modp->unlinkFrBack();
            modps.ತಳ್ಳು_back(modp);
        }

        // Fix-up levels
        ಧ್ರುವ VNUser1InUse user1InUse;
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ಧ್ರುವ Ast_ಗಂಟು_Module* ap, ಧ್ರುವ Ast_ಗಂಟು_Module* bp) {
                             ಪ್ರತ್ಯಾಗಮ ap->level() < bp->level();
                         });

        // Re-insert modules
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp : modps) netlistp->addModulesp(modp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Param_ತುದಿ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ಧ್ರುವ ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ಧ್ರುವ ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ಗಂಟು_FTask_Ref* ftaskrefp) {
            Ast_ಗಂಟು_FTask* ftaskp = ftaskrefp->taskp();
            ಯದಿ (!ftaskp || !ftaskp->classMethod()) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ಧ್ರುವ auto ancestorClassOf = [](Ast_ಗಂಟು* startp) -> AstClass* {
                ಪ್ರತಿ (Ast_ಗಂಟು* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    ಯದಿ (AstClass* ಧ್ರುವ cp = VN_CAST(np, Class)) ಪ್ರತ್ಯಾಗಮ cp;
                }
                ಪ್ರತ್ಯಾಗಮ nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            ಯದಿ (refClassp == ftaskrefp->classOrPackagep())
                ಪ್ರತ್ಯಾಗಮ;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            ಯದಿ (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (classp->user3p()) ಪ್ರತ್ಯಾಗಮ;  // will not get removed, no need to relink
            AstClass* ಧ್ರುವ parametrizedClassp = VN_CAST(classp->user4p(), Class);
            ಯದಿ (!parametrizedClassp) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ಗಂಟು_FTask* ftaskp) {
                ಯದಿ (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಯದಿ (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ಧ್ರುವ ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        ಪ್ರತಿ (AstClass* ಧ್ರುವ classp : m_state.m_paramClasses) {
            ಯದಿ (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                ಯದಿ (V3Error::error_ಎಣಿಕೆ()) ಅನುವರ್ತ;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(classp->unlinkFrBack()), classp);
            } ಅನ್ಯಥಾ {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(ಅಸತ್ಯ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ಗಂಟು::user3_Clear_ವೃಕ್ಷ();
        ಪ್ರತಿ (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ಗಂಟು* ಧ್ರುವ nodep) { nodep->user3(ಸತ್ಯ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ಗಂಟು* ಧ್ರುವ nodep) {
            nodep->foreachLink([&](Ast_ಗಂಟು** ಧ್ರುವ linkpp, ಧ್ರುವ ವರ್ಣ* namep) {
                ಯದಿ (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_ತುದಿ() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(Param_ತುದಿ);
};

//######################################################################
// Param class functions

ಶೂನ್ಯ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    ಯದಿ (dump_ವೃಕ್ಷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_ತುದಿ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    ಯದಿ (dump_ವೃಕ್ಷ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ವೃಕ್ಷ("param", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : rootp->deferredParamVarps()) {
        ಯದಿ (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
