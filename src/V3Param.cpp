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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Param.h"

#অন্তর্ভাব "V3Case.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3HierBlock.h"
#অন্তর্ভাব "V3LinkDotIfaceCapture.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3Parse.h"
#অন্তর্ভাব "V3Simulate.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3Unroll.h"
#অন্তর্ভাব "V3Width.h"

#অন্তর্ভাব <cctype>
#অন্তর্ভাব <cstdlib>
#অন্তর্ভাব <deque>
#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
স্থির AstClassRefDType* class_Ref_DType_Of_গ্রন্থি(Ast_গ্রন্থি* nodep) {
    যখন (AstTypedef* ধ্রুবক tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_গ্রন্থি_DType* ধ্রুবক dtp = VN_CAST(nodep, গ্রন্থি_DType);
    ফেরত dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

শ্রেণী ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ধ্রুবক V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ধ্রুবক std::string, Ast_গ্রন্থি_Module*>;
    using ParamConstMap = std::map<ধ্রুবক std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ধ্রুবক std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ধ্রুবক যৌক্তিক m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<ধ্রুবক V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ধ্রুবক std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<ধ্রুবক AstVar*> m_untypedParams;

    // METHODS

সর্বজনীন:
    ParameterizedHierBlocks(ধ্রুবক V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        জন্য (ধ্রুবক auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ধ্রুবক V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            জন্য (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    নতুন File_সারি{File_সারি::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ধ্রুবক যৌক্তিক inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ঢোকাও({hierOpt.second.origName(), {}});
        }
        জন্য (Ast_গ্রন্থি_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
            যদি (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ধ্রুবক string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ধ্রুবক auto defParamIt = m_modParams.find(actualModName);
            যদি (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                        যদি (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ধ্রুবক AstBasicDType* ধ্রুবক dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        যদি (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    যৌক্তিক hierSubRun() ধ্রুবক { ফেরত m_hierSubRun; }
    যৌক্তিক isHierBlock(ধ্রুবক string& origName) ধ্রুবক {
        ফেরত m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_গ্রন্থি_Module* findByParams(ধ্রুবক string& origName, AstPin* firstPinp,
                                ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ধ্রুবক std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        ধ্রুবক auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ধ্রুবক AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        জন্য (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            যৌক্তিক found = সত্য;
            আকার_t paramIdx = 0;
            ধ্রুবক ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.আকার_2() == hierIt->second->params().আকার_2(), "not match");
            জন্য (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (!pinp->exprp()) চালু;
                যদি (ধ্রুবক AstVar* ধ্রুবক modvarp = pinp->modVarp()) {
                    AstConst* ধ্রুবক constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ধ্রুবক auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ধ্রুবক যৌক্তিক untyped = m_untypedParams.গুনতি(paramIt->second);
                    AstConst* ধ্রুবক defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    যদি (defValuep && areSame(constp, defValuep)) {
                        যদি (!untyped || constp->same_মান_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            চালু;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ধ্রুবক auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    যদি (pIt == params.end() || paramIdx >= params.আকার_2()
                        || !areSame(constp, pIt->second.get())) {
                        found = মিথ্যা;
                        বিরতি;
                    }
                    যদি (untyped && !constp->same_মান_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = মিথ্যা;
                        বিরতি;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            যদি (found && paramIdx == hierIt->second->params().আকার_2()) বিরতি;
        }
        যদি (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_গ্রন্থি::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            ফেরত nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ধ্রুবক HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ধ্রুবক auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        যদি (it == m_hierBlockMod.end()) ফেরত nullptr;
        ফেরত it->second;
    }
    স্থির যৌক্তিক areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        যদি (pinValuep->isString()) {
            ফেরত pinValuep->অঙ্ক().toString() == hierOptParamp->অঙ্ক().toString();
        }

        যদি (hierOptParamp->isDouble()) {
            দ্বিগুণ var;
            যদি (pinValuep->isDouble()) {
                var = pinValuep->অঙ্ক().toDouble();
            } অন্যথা {  // Cast from integer to real
                V3_সংখ্যা var_অঙ্ক{pinValuep, V3_সংখ্যা::Double{}, 0.0};
                var_অঙ্ক.opIToRD(pinValuep->অঙ্ক());
                var = var_অঙ্ক.toDouble();
            }
            ফেরত V3_সংখ্যা::epsilonEqual(var, hierOptParamp->অঙ্ক().toDouble());
        } অন্যথা {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_সংখ্যা var_অঙ্ক{pinValuep, hierOptParamp->অঙ্ক().width()};
            যদি (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_অঙ্ক.opRToIRoundS(pinValuep->অঙ্ক());
            } অন্যথা যদি (pinValuep->isSigned()) {
                var_অঙ্ক.opExtendS(pinValuep->অঙ্ক(), pinValuep->অঙ্ক().width());
            } অন্যথা {
                var_অঙ্ক.opAssign(pinValuep->অঙ্ক());
            }
            V3_সংখ্যা isEq{pinValuep, 1};
            isEq.opEq(var_অঙ্ক, hierOptParamp->অঙ্ক());
            ফেরত isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

শ্রেণী ParamProcessor final {
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
    ধ্রুবক VNUser2InUse m_inuser2;
    ধ্রুবক VNUser3InUse m_inuser3;
    ধ্রুবক VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, Ast_গ্রন্থি*>;
    কাঠামো ModInfo final {
        Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_গ্রন্থি_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ধ্রুবক std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ধ্রুবক std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ধ্রুবক std::string, std::string> m_মান_Names;  // Parameter value text to its name
    std::set<std::string> m_মান_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    স্থির constexpr std::string::আকার_type DIGEST_DIGITS = 8;

    ধ্রুবক Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_মান_Map = std::map<std::string, Ast_গ্রন্থি*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_গ্রন্থি_Module*, Default_মান_Map> m_default_Parameter_মানসমূহ;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ধ্রুবক Ast_গ্রন্থি_Module*, ধ্রুবক AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, পূর্ণসংখ্যা>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, পূর্ণসংখ্যা> m_param_সূচক;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ধ্রুবক AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    স্থির শূন্য makeSmallNames(Ast_গ্রন্থি_Module* modp) {
        std::vector<পূর্ণসংখ্যা> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isGParam() || varp->isIfaceRef()) {
                    অক্ষর ch = varp->name()[0];
                    ch = std::toupper(ch);
                    যদি (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<পূর্ণসংখ্যা>(ch)] * 256 + ch);
                    usedLetter[static_cast<পূর্ণসংখ্যা>(ch)]++;
                }
            } অন্যথা যদি (AstParamTypeDType* ধ্রুবক typep = VN_CAST(stmtp, ParamTypeDType)) {
                ধ্রুবক অক্ষর ch = 'T';
                typep->user3(usedLetter[static_cast<পূর্ণসংখ্যা>(ch)] * 256 + ch);
                usedLetter[static_cast<পূর্ণসংখ্যা>(ch)]++;
            }
        }
    }
    স্থির string paramSmallName(Ast_গ্রন্থি_Module* modp, Ast_গ্রন্থি* varp) {
        যদি (varp->user3() <= 1) makeSmallNames(modp);
        পূর্ণসংখ্যা সূচক = varp->user3() / 256;
        ধ্রুবক অক্ষর ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        যখন (সূচক) {
            st += cvtToStr(static_cast<অক্ষর>((সূচক % 25) + 'A'));
            সূচক /= 26;
        }
        ফেরত st;
    }

    স্থির যৌক্তিক is_Aggregate_Param_মান(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ফেরত VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    স্থির string param_মান_String(ধ্রুবক Ast_গ্রন্থি* nodep) {
        যদি (ধ্রুবক AstRefDType* ধ্রুবক refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string চাবি = nodep->name();
        যদি (ধ্রুবক AstIfaceRefDType* ধ্রুবক ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            যদি (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                চাবি = ifrtp->cellp()->modp()->name();
            } অন্যথা যদি (ifrtp->ifacep()) {
                চাবি = ifrtp->ifacep()->name();
            } অন্যথা {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক dtypep
                   = VN_CAST(nodep, গ্রন্থি_UOr_Struct_DType)) {
            চাবি += " ";
            চাবি += dtypep->verilogKwd();
            চাবি += " {";
            জন্য (ধ্রুবক Ast_গ্রন্থি* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                চাবি += param_মান_String(memberp);
                চাবি += ";";
            }
            চাবি += "}";
        } অন্যথা যদি (ধ্রুবক AstMemberDType* ধ্রুবক dtypep = VN_CAST(nodep, MemberDType)) {
            চাবি += " ";
            চাবি += param_মান_String(dtypep->subDTypep());
        } অন্যথা যদি (ধ্রুবক AstBasicDType* ধ্রুবক dtypep = VN_CAST(nodep, BasicDType)) {
            যদি (dtypep->isSigned()) চাবি += " signed";
            যদি (dtypep->isRanged()) {
                চাবি += "[" + cvtToStr(dtypep->বাম()) + ":" + cvtToStr(dtypep->ডান()) + "]";
            }
        } অন্যথা যদি (ধ্রুবক AstPackArrayDType* ধ্রুবক dtypep = VN_CAST(nodep, PackArrayDType)) {
            চাবি += "[";
            চাবি += cvtToStr(dtypep->বাম());
            চাবি += ":";
            চাবি += cvtToStr(dtypep->ডান());
            চাবি += "] ";
            চাবি += param_মান_String(dtypep->subDTypep());
        } অন্যথা যদি (ধ্রুবক AstPattern* ধ্রুবক patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            চাবি += "'{";
            জন্য (ধ্রুবক AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                যদি (memp->isDefault()) চাবি += "default";
                যদি (ধ্রুবক AstText* ধ্রুবক textp = VN_CAST(memp->keyp(), Text)) {
                    চাবি += textp->text();
                } অন্যথা যদি (memp->keyp()) {
                    চাবি += param_মান_String(memp->keyp());
                }
                চাবি += ":";
                যদি (memp->repp()) চাবি += param_মান_String(memp->repp()) + "x";
                জন্য (ধ্রুবক Ast_গ্রন্থি_রাশিমালা* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), গ্রন্থি_রাশিমালা)) {
                    চাবি += param_মান_String(valuep) + ";";
                }
                চাবি += ",";
            }
            চাবি += "}";
        } অন্যথা যদি (ধ্রুবক AstConsArray* ধ্রুবক consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            চাবি += "{";
            জন্য (ধ্রুবক AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                চাবি += cvtToStr(itemp->সূচক()) + ":" + param_মান_String(itemp->valuep()) + ",";
            }
            যদি (consArrayp->defaultp()) {
                চাবি += "default:" + param_মান_String(consArrayp->defaultp()) + ",";
            }
            চাবি += "}";
        } অন্যথা যদি (ধ্রুবক AstConsStruct* ধ্রুবক consp = VN_CAST(nodep, ConsStruct)) {
            চাবি += "{";
            জন্য (ধ্রুবক AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                চাবি += param_মান_String(itemp->valuep());
                চাবি += ",";
            }
            চাবি += "}";
        } অন্যথা যদি (ধ্রুবক AstClassRefDType* ধ্রুবক classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            যদি (classRefp->classp()) {
                ধ্রুবক string& className = classRefp->classp()->name();
                ধ্রুবক string& origName = classRefp->classp()->origName();
                ধ্রুবক যৌক্তিক isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                যদি (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } অন্যথা যদি (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    চাবি = className;
                } অন্যথা {
                    // Unspecialized class with no params - use origName
                    চাবি = origName;
                }
            } অন্যথা {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } অন্যথা যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            চাবি = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } অন্যথা যদি (ধ্রুবক AstQueueDType* ধ্রুবক dtypep = VN_CAST(nodep, QueueDType)) {
            চাবি = paramElemString(dtypep->subDTypep()) + "$[$";
            যদি (dtypep->boundConst()) চাবি += ":" + cvtToStr(dtypep->boundConst());
            চাবি += "]";
        } অন্যথা যদি (ধ্রুবক AstDynArrayDType* ধ্রুবক dtypep = VN_CAST(nodep, DynArrayDType)) {
            চাবি = paramElemString(dtypep->subDTypep()) + "$[]";
        } অন্যথা যদি (ধ্রুবক AstAssocArrayDType* ধ্রুবক dtypep = VN_CAST(nodep, AssocArrayDType)) {
            চাবি = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->চাবি_DTypep()) + "]";
        } অন্যথা যদি (ধ্রুবক AstWildcardArrayDType* ধ্রুবক dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            চাবি = paramElemString(dtypep->subDTypep()) + "$[*]";
        } অন্যথা যদি (ধ্রুবক AstEnumDType* ধ্রুবক dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            চাবি += " enum {";
            জন্য (ধ্রুবক AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(itemp->valuep(), Const);
                চাবি += itemp->name() + "=" + (constp ? constp->অঙ্ক().ascii() : "?") + ";";
            }
            চাবি += "}";
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = VN_CAST(nodep, গ্রন্থি_DType)) {
            চাবি += dtypep->prettyDTypeName(সত্য);
        }
        UASSERT_OBJ(!চাবি.empty(), nodep, "Parameter yielded no value string");
        ফেরত চাবি;
    }
    // As paramValueString(), for the element or key type of a container
    স্থির string paramElemString(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ধ্রুবক AstClassRefDType* ধ্রুবক classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        যদি (classRefp && classRefp->paramsp()) ফেরত classRefp->prettyDTypeName(সত্য);
        ফেরত param_মান_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    স্থির string digestSuffix(ধ্রুবক string& text, std::set<string>& usedr) {
        ধ্রুবক string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::আকার_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        যখন (digits < hex.আকার_2() && !usedr.ঢোকাও(hex.substr(0, digits)).second) ++digits;
        ফেরত hex.substr(0, digits);
    }
    string param_মান_সংখ্যা(Ast_গ্রন্থি* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        যদি (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ধ্রুবক string text = param_মান_String(nodep);
        ধ্রুবক auto pair = m_মান_Names.emplace(text, "");
        যদি (pair.second) pair.first->second = "z" + digestSuffix(text, m_মান_Suffixes);
        ফেরত pair.first->second;
    }
    string moduleCalcName(ধ্রুবক Ast_গ্রন্থি_Module* srcModp, ধ্রুবক string& longname) {
        string newname = longname;
        যদি (longname.দৈর্ঘ্য() > 30) {
            ধ্রুবক auto pair = m_longMap.emplace(longname, "");
            যদি (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        ফেরত newname;
    }
    স্থির Ast_গ্রন্থি_DType* arraySubDTypep(Ast_গ্রন্থি_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            ফেরত adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        যদি (ধ্রুবক AstBracketArrayDType* ধ্রুবক adtypep = VN_CAST(nodep, BracketArrayDType)) {
            ফেরত adtypep->subDTypep();
        }
        ফেরত nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    স্থির Ast_গ্রন্থি_DType* arraySubDTypeDeepp(Ast_গ্রন্থি_DType* nodep) {
        যখন (Ast_গ্রন্থি_DType* ধ্রুবক subp = arraySubDTypep(nodep)) nodep = subp;
        ফেরত nodep;
    }
    স্থির যৌক্তিক isString(Ast_গ্রন্থি_DType* nodep) {
        যদি (AstBasicDType* ধ্রুবক basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            ফেরত basicp->isString();
        ফেরত মিথ্যা;
    }
    শূন্য collectPins(CloneMap* clonemapp, Ast_গ্রন্থি_Module* modp, যৌক্তিক originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ধ্রুবক Ast_গ্রন্থি* originalParamp = nullptr;
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } অন্যথা যদি (AstParamTypeDType* ধ্রুবক ptp = VN_CAST(stmtp, ParamTypeDType)) {
                যদি (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            যদি (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            যদি (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    শূন্য relinkPins(ধ্রুবক CloneMap* clonemapp, AstPin* startpinp) {
        জন্য (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ধ্রুবক auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } অন্যথা যদি (pinp->modPTypep()) {
                ধ্রুবক auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } অন্যথা {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    শূন্য relinkPinsByName(AstPin* startpinp, Ast_গ্রন্থি_Module* modp) {
        std::map<ধ্রুবক string, AstVar*> nameToPin;
        জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        জন্য (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (ধ্রুবক AstVar* ধ্রুবক varp = pinp->modVarp()) {
                ধ্রুবক auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    স্থির যৌক্তিক hasDescendantDefparams(ধ্রুবক Ast_গ্রন্থি_Module* modp,
                                       std::set<ধ্রুবক Ast_গ্রন্থি_Module*>& visited) {
        যদি (!visited.ঢোকাও(modp).second) ফেরত মিথ্যা;
        জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ধ্রুবক cellp = VN_CAST(stmtp, Cell);
            যদি (!cellp) চালু;
            জন্য (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    ফেরত সত্য;
                }
            }
            যদি (Ast_গ্রন্থি_Module* ধ্রুবক childModp = cellp->modp()) {
                যদি (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    ফেরত সত্য;
                }
            }
        }
        visited.erase(modp);
        ফেরত মিথ্যা;
    }
    স্থির যৌক্তিক hasDescendantDefparams(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        std::set<ধ্রুবক Ast_গ্রন্থি_Module*> visited;
        ফেরত hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ধ্রুবক AstPin* nestedDefparamp(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        ধ্রুবক auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        যদি (!pair.second) ফেরত pair.first->second;  // Searched, or being searched
        ধ্রুবক AstPin* foundp = nullptr;
        modp->exists([&](ধ্রুবক AstCell* cellp) {
            জন্য (ধ্রুবক AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (!pinp->paramPath().empty()) foundp = pinp;
            }
            যদি (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            ফেরত foundp != nullptr;
        });
        pair.first->second = foundp;
        ফেরত foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    শূন্য checkSupportedParam(Ast_গ্রন্থি_Module* modp, AstPin* pinp) ধ্রুবক {
        // ConsArray is not supported because that can not be set via -G
        // option.
        যদি (pinp->modVarp()) {
            যৌক্তিক supported = মিথ্যা;
            যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(pinp->exprp(), Const)) {
                supported = সত্য;
                যদি (constp->অঙ্ক().isString()
                    && !V3HierBlock::stringParamPassable(constp->অঙ্ক().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            যদি (!supported) {
                pinp->v3error(
                    Ast_গ্রন্থি::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } অন্যথা যদি (ধ্রুবক AstParamTypeDType* ধ্রুবক typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_গ্রন্থি::prettyNameQ(modp->origName()));
        }
    }
    যৌক্তিক moduleExists(ধ্রুবক string& modName) ধ্রুবক {
        যদি (m_allModuleNames.find(modName) != m_allModuleNames.end()) ফেরত সত্য;
        যদি (m_modNameMap.find(modName) != m_modNameMap.end()) ফেরত সত্য;
        ফেরত মিথ্যা;
    }

    string parameterizedHierBlockName(Ast_গ্রন্থি_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_গ্রন্থি*> pins;

        AstPin* pinp = paramPinsp;
        যখন (pinp) {
            checkSupportedParam(modp, pinp);
            যদি (ধ্রুবক AstVar* ধ্রুবক varp = pinp->modVarp()) {
                যদি (!pinp->exprp()) চালু;
                যদি (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } অন্যথা যদি (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), গ্রন্থি_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ধ্রুবক auto pair = m_default_Parameter_মানসমূহ.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        যদি (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            যদি (ধ্রুবক AstPin* ধ্রুবক defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_গ্রন্থি::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_মান_Map params;
            জন্য (Ast_গ্রন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                    যদি (varp->isGParam()) {
                        AstConst* ধ্রুবক constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } অন্যথা যদি (AstParamTypeDType* ধ্রুবক p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_গ্রন্থি* ধ্রুবক dtypep = static_cast<Ast_গ্রন্থি*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ধ্রুবক auto paramsIt = pair.first;
        যদি (paramsIt->second.empty()) ফেরত modp->origName();  // modp has no parameter

        string longname = modp->origName();
        জন্য (auto&& default_মান : paramsIt->second) {
            ধ্রুবক auto pinIt = pins.find(default_মান.first);
            // If the pin does not have a value assigned, use the default one.
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep = pinIt == pins.end() ? default_মান.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_মান.first + "=";
            // constp can be nullptr

            যদি (ধ্রুবক AstConst* ধ্রুবক p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->অঙ্ক().ascii(মিথ্যা);
            } অন্যথা যদি (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_বৃক্ষ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ধ্রুবক auto iter = m_longMap.find(longname);
        যদি (iter != m_longMap.end()) ফেরত iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ঢোকাও(longname);
        যখন (সত্য) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ধ্রুবক string hashStr = hashStrGen.digestHex();
            জন্য (string::আকার_type i = 1; i < hashStr.আকার_2(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                যদি (newName.at(newName.আকার_2() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                যদি (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    ফেরত newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ঢোকাও(V3Os::trueRandom(64));
        }
    }
    শূন্য replaceRefsRecurse(Ast_গ্রন্থি* ধ্রুবক nodep, ধ্রুবক AstClass* ধ্রুবক oldClassp,
                            AstClass* ধ্রুবক newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        যদি (AstClassRefDType* ধ্রুবক classRefp = VN_CAST(nodep, ClassRefDType)) {
            যদি (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } অন্যথা যদি (AstClassOrPackageRef* ধ্রুবক classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            যদি (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } অন্যথা যদি (AstTypedef* ধ্রুবক typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            যদি (typedefp->subDTypep()) {
                যদি (AstClassRefDType* ধ্রুবক classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    যদি (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } অন্যথা যদি (Ast_গ্রন্থি_FTask_Ref* ধ্রুবক ftaskRefp = VN_CAST(nodep, গ্রন্থি_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            যদি (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            যদি (Ast_গ্রন্থি_FTask* ধ্রুবক oldTaskp = ftaskRefp->taskp()) {
                যদি (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        যদি (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        যদি (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        যদি (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        যদি (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        যদি (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    শ্রেণী VarXRefRelinkVisitor final : সর্বজনীন VNVisitor {
        Ast_গ্রন্থি_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_গ্রন্থি_Module*> m_varModuleMap;  // Cache var->module lookups

    সর্বজনীন:
        explicit VarXRefRelinkVisitor(Ast_গ্রন্থি_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ব্যক্তিগত:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_গ্রন্থি_Module* findVarModule(AstVar* varp) {
            ধ্রুবক auto it = m_varModuleMap.find(varp);
            যদি (it != m_varModuleMap.end()) ফেরত it->second;
            Ast_গ্রন্থি_Module* varModp = nullptr;
            জন্য (Ast_গ্রন্থি* np = varp; np; np = np->backp()) {
                যদি (Ast_গ্রন্থি_Module* ধ্রুবক modp = VN_CAST(np, গ্রন্থি_Module)) {
                    varModp = modp;
                    বিরতি;
                }
            }
            m_varModuleMap[varp] = varModp;
            ফেরত varModp;
        }

        শূন্য visit(AstVarXRef* nodep) override {
            AstVar* ধ্রুবক varp = nodep->varp();
            যদি (!varp) {
                iterateChildren(nodep);
                ফেরত;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ধ্রুবক string& dotted = nodep->dotted();
            যদি (dotted.empty()) {
                iterateChildren(nodep);
                ফেরত;
            }

            ধ্রুবক আকার_t dotPos = dotted.find('.');
            ধ্রুবক string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            যদি (portName.empty()) {
                iterateChildren(nodep);
                ফেরত;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            জন্য (Ast_গ্রন্থি* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (AstVar* ধ্রুবক varChkp = VN_CAST(stmtp, Var)) {
                    যদি (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        বিরতি;
                    }
                }
            }
            যদি (!portVarp) {
                iterateChildren(nodep);
                ফেরত;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ধ্রুবক irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            যদি (!irefp) {
                iterateChildren(nodep);
                ফেরত;
            }

            Ast_গ্রন্থি_Module* ধ্রুবক newIfacep = irefp->ifaceViaCellp();
            যদি (!newIfacep) {
                iterateChildren(nodep);
                ফেরত;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_গ্রন্থি_Module* ধ্রুবক varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            যদি (varModp && varModp != newIfacep) {
                জন্য (Ast_গ্রন্থি* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstVar* ধ্রুবক newVarp = VN_CAST(stmtp, Var)) {
                        যদি (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            বিরতি;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    শূন্য fixupCrossModuleRefDTypes(Ast_গ্রন্থি_Module* newModp) {
        যদি (!V3LinkDotIfaceCapture::enabled()) ফেরত;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        পূর্ণসংখ্যা fixed_গুনতি = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ধ্রুবক VIfaceCaptureTag* ধ্রুবক tagp = V3LinkDotIfaceCapture::captureTag(refp);
            যদি (!tagp) ফেরত;

            Ast_গ্রন্থি_Module* ধ্রুবক correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            যদি (!correctModp || correctModp->dead()) ফেরত;
            যদি (correctModp->parameterizedTemplate()) ফেরত;

            যৌক্তিক fixed = মিথ্যা;
            যদি (refp->typedefp()) {
                যদি (AstTypedef* ধ্রুবক newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    যদি (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = সত্য;
                }
            }
            যদি (!fixed && refp->refDTypep()) {
                যদি (Ast_গ্রন্থি_DType* ধ্রুবক newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = সত্য;
                }
            }
            যদি (fixed) ++fixed_গুনতি;
        });
        V3Stats::add_Stat_যোগ("IfaceCapture, Ledger fixups in V3Param", fixed_গুনতি);
    }

    // Return true on success, false on error
    যৌক্তিক deepCloneModule(Ast_গ্রন্থি_Module* srcModp, Ast_গ্রন্থি* ifErrorp, AstPin* paramsp,
                         ধ্রুবক string& newname, ধ্রুবক IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_গ্রন্থি_Module* newModp;
        যদি (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_বৃক্ষ(মিথ্যা), গ্রন্থি_Module);
        } অন্যথা {
            newModp = srcModp->clone_বৃক্ষ(মিথ্যা);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        যদি (AstIface* ধ্রুবক newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(সত্য);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(মিথ্যা);

        newModp->name(newname);
        newModp->user2(মিথ্যা);  // We need to re-recurse this module once changed
        newModp->recursive(মিথ্যা);
        newModp->recursiveClone(মিথ্যা);
        newModp->hasGParam(মিথ্যা);
        // Recursion may need level cleanups
        যদি (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        যদি ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_বৃক্ষ(), newModp);
            ফেরত মিথ্যা;
        }

        যদি (AstClass* ধ্রুবক newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_গ্রন্থি* insertp = srcModp;
        যখন (VN_IS(insertp->nextp(), গ্রন্থি_Module)
               && VN_AS(insertp->nextp(), গ্রন্থি_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->add_পরবর্তী_Here(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ধ্রুবক auto iter = m_modNameMap.find(newname);
        CloneMap* ধ্রুবক clonemapp = &(iter->second.m_cloneMap);
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
        যৌক্তিক needRelinkByName = মিথ্যা;
        যদি (paramsp) {
            জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = সত্য;
                    বিরতি;
                }
            }
        }
        যদি (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } অন্যথা {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        জন্য (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ধ্রুবক AstIfaceRefDType* ধ্রুবক portIrefp = it->first;
            ধ্রুবক AstIfaceRefDType* ধ্রুবক pinIrefp = it->second;
            AstIfaceRefDType* ধ্রুবক cloneIrefp = portIrefp->clonep();
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
        যদি (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (pinp->exprp()) {
                যদি (AstVar* ধ্রুবক modvarp = pinp->modVarp()) {
                    Ast_গ্রন্থি* ধ্রুবক newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ধ্রুবক exprp = VN_CAST(newp, Const);
                    AstConst* ধ্রুবক origp = VN_CAST(modvarp->valuep(), Const);
                    ধ্রুবক যৌক্তিক overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    যদি (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_বৃক্ষ(মিথ্যা));
                    modvarp->overriddenParam(overridden);
                } অন্যথা যদি (AstParamTypeDType* ধ্রুবক modptp = pinp->modPTypep()) {
                    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = VN_AS(pinp->exprp(), গ্রন্থি_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    যদি (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_বৃক্ষ();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_বৃক্ষ(মিথ্যা));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        ফেরত সত্য;
    }
    ধ্রুবক ModInfo* moduleFindOrClone(Ast_গ্রন্থি_Module* srcModp, Ast_গ্রন্থি* ifErrorp, AstPin* paramsp,
                                     ধ্রুবক string& newname, ধ্রুবক IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        যদি (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } অন্যথা {
            যদি (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                ফেরত nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ধ্রুবক ModInfo* ধ্রুবক modInfop = &(it->second);
        ফেরত modInfop;
    }

    শূন্য convertToStringp(Ast_গ্রন্থি* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ধ্রুবক constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        যদি (constp && !constp->অঙ্ক().isString()) {
            constp->replaceWith(
                নতুন AstConst{constp->fileline(), AstConst::String{}, constp->অঙ্ক().toString()});
            VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    শূন্য constify_Member_মান(AstVar* varp, std::set<AstVar*>& inProgress) {
        যদি (!varp->valuep() || VN_IS(varp->valuep(), Const)) ফেরত;
        যদি (!inProgress.ঢোকাও(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
            varp->valuep(নতুন AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            ফেরত;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ধ্রুবক AstVarRef* refp) {
            AstVar* ধ্রুবক refVarp = refp->varp();
            যদি (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ঢোকাও(refVarp);
            }
        });
        জন্য (AstVar* ধ্রুবক sibp : siblings) constify_Member_মান(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    স্থির AstParseRef* memberParseRef(Ast_গ্রন্থি* nodep) {
        যদি (AstParseRef* ধ্রুবক refp = VN_CAST(nodep, ParseRef)) ফেরত refp;
        Ast_গ্রন্থি_Pre_Sel* ধ্রুবক selp = VN_CAST(nodep, গ্রন্থি_Pre_Sel);
        ফেরত selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    স্থির শূন্য replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } অন্যথা {
            Ast_গ্রন্থি_Pre_Sel* ধ্রুবক selp = VN_AS(dotp->rhsp(), গ্রন্থি_Pre_Sel);
            যদি (AstAttrOf* ধ্রুবক attrp = selp->attrp()) {
                যদি (Ast_গ্রন্থি* ধ্রুবক oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_বৃক্ষ(মিথ্যা));
                    VL_DO_DANGLING(oldFromp->delete_বৃক্ষ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_বৃক্ষ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    স্থির যৌক্তিক isDeferredMemberBase(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (VN_IS(nodep, MemberSel)) ফেরত সত্য;
        যদি (ধ্রুবক Ast_গ্রন্থি_Pre_Sel* ধ্রুবক selp = VN_CAST(nodep, গ্রন্থি_Pre_Sel)) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDTypep = selp->fromp()->dtypep();
            ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), গ্রন্থি_Array_DType) : nullptr;
            যদি (arrayDTypep) {
                ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক elemDTypep = arrayDTypep->subDTypep();
                ফেরত elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), গ্রন্থি_UOr_Struct_DType);
            }
            ফেরত isDeferredMemberBase(selp->fromp());
        }
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->dtypep();
        ফেরত dtypep && VN_IS(dtypep->skipRefOrNullp(), গ্রন্থি_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    স্থির শূন্য resolveMemberDot(AstDot* dotp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = VN_CAST(dotp->lhsp(), গ্রন্থি_রাশিমালা);
        AstParseRef* ধ্রুবক memberRefp = memberParseRef(dotp->rhsp());
        যদি (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) ফেরত;
        AstMemberSel* ধ্রুবক newp = নতুন AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    শূন্য resolveDotToTypedef(Ast_গ্রন্থি* exprp) {
        AstDot* ধ্রুবক dotp = VN_CAST(exprp, Dot);
        যদি (!dotp) ফেরত;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ধ্রুবক classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        যদি (!classRefp) {
            resolveMemberDot(dotp);
            ফেরত;
        }
        AstParseRef* ধ্রুবক parseRefp = memberParseRef(dotp->rhsp());
        যদি (!parseRefp) ফেরত;

        ধ্রুবক AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ধ্রুবক paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        যদি (paramTypep) {
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = paramTypep->subDTypep();
            AstClassRefDType* ধ্রুবক classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            যদি (classRefDTypep) {
                AstClass* ধ্রুবক srcClassp = classRefDTypep->classp();
                যদি (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    যদি (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        যদি (classRefp->paramsp()) {
            AstClass* ধ্রুবক srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            যদি (srcClassp && srcClassp->hasGParam()) {
                যদি (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        যদি (AstClassRefDType* ধ্রুবক crp = class_Ref_DType_Of_গ্রন্থি(classRefp->classOrPackageNodep())) {
            যদি (AstClass* ধ্রুবক srcClassp = crp->classp()) {
                যদি (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    যদি (AstClass* ধ্রুবক newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        যদি (!lhsClassp) ফেরত;

        Ast_গ্রন্থি* ধ্রুবক memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        যদি (AstTypedef* ধ্রুবক tdefp = VN_CAST(memberp, Typedef)) {
            যদি (parseRefp != dotp->rhsp()) ফেরত;
            AstRefDType* ধ্রুবক refp = নতুন AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
        } অন্যথা যদি (AstVar* ধ্রুবক varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    শূন্য substituteParamMember(AstDot* ধ্রুবক dotp, AstParseRef* ধ্রুবক memberRefp,
                               AstVar* ধ্রুবক varp) {
        যদি (!varp->isParam() || !varp->valuep()) ফেরত;
        যদি (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_মান(varp, inProgress);
        }
        AstConst* ধ্রুবক constp = VN_CAST(varp->valuep(), Const);
        যদি (!constp) ফেরত;
        AstConst* ধ্রুবক newp = constp->clone_বৃক্ষ(মিথ্যা);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    শূন্য resolveParamClassRefDType(AstRefDType* refp) {
        যদি (refp->typedefp() || refp->refDTypep()) ফেরত;

        AstClassOrPackageRef* ধ্রুবক classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        যদি (!classRefp) ফেরত;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        যদি (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            ফেরত;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_গ্রন্থি* targetp = classRefp->classOrPackageNodep();
        যখন (ধ্রুবক AstTypedef* ধ্রুবক tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtargetp = VN_CAST(targetp, গ্রন্থি_DType))
            targetp = dtargetp->skipRefOrNullp();
        যদি (!targetp) ফেরত;
        AstClassRefDType* ধ্রুবক classRefDTypep = VN_CAST(targetp, ClassRefDType);
        যদি (!classRefDTypep) ফেরত;
        srcClassp = classRefDTypep->classp();
        যদি (!srcClassp) ফেরত;

        AstClass* newClassp = srcClassp;
        যদি (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ধ্রুবক typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        যদি (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    স্থির যৌক্তিক defaultParamsHaveTypeQueryOnParamType(ধ্রুবক AstClassRefDType* nodep) {
        যৌক্তিক found = মিথ্যা;
        nodep->foreach([&](ধ্রুবক AstAttrOf* attrp) {
            যদি (found || !attrp->attrType().isTypeQuery()) ফেরত;
            ধ্রুবক AstRefDType* ধ্রুবক refp = VN_CAST(attrp->fromp(), RefDType);
            যদি (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = সত্য;
        });
        ফেরত found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    যৌক্তিক classTypeMatchesDefaultClone(ধ্রুবক Ast_গ্রন্থি_DType* exprp, ধ্রুবক Ast_গ্রন্থি_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ধ্রুবক auto* ধ্রুবক রাশিমালা_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ধ্রুবক auto* ধ্রুবক origClassRefp = VN_CAST(origp, ClassRefDType);
        যদি (!রাশিমালা_Class_Refp || !origClassRefp) ফেরত মিথ্যা;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        যদি (defaultClonep && defaultClonep == রাশিমালা_Class_Refp->classp()) ফেরত সত্য;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        যদি (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) ফেরত মিথ্যা;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        যদি (!origClassRefp->classp()->hasGParam()) ফেরত মিথ্যা;
        যদি (origClassRefp->classp()->origName() != রাশিমালা_Class_Refp->classp()->origName())
            ফেরত মিথ্যা;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        যদি (!m_defaultCloneInProgress.ঢোকাও(origClassRefp->classp()).second) ফেরত মিথ্যা;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ধ্রুবক origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_বৃক্ষ(মিথ্যা));
        Ast_গ্রন্থি_Module* ধ্রুবক resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ধ্রুবক যৌক্তিক match = resolvedModp && VN_CAST(resolvedModp, Class) == রাশিমালা_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_বৃক্ষ(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        ফেরত match;
    }

    স্থির যৌক্তিক paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        যদি (exprp->অঙ্ক().width() == origp->অঙ্ক().width()) ফেরত মিথ্যা;
        যদি (exprp->অঙ্ক().isOpaque()) ফেরত মিথ্যা;
        যদি (origp->অঙ্ক().isOpaque()) ফেরত মিথ্যা;
        ধ্রুবক পূর্ণসংখ্যা maxWidth = std::max(exprp->অঙ্ক().width(), origp->অঙ্ক().width());
        V3_সংখ্যা রাশিমালা_অঙ্ক{exprp, maxWidth};
        যদি (exprp->অঙ্ক().isSigned()) {
            রাশিমালা_অঙ্ক.opExtendS(exprp->অঙ্ক(), exprp->অঙ্ক().width());
        } অন্যথা {
            রাশিমালা_অঙ্ক.opAssign(exprp->অঙ্ক());
        }
        V3_সংখ্যা orig_অঙ্ক{origp, maxWidth};
        যদি (origp->অঙ্ক().isSigned()) {
            orig_অঙ্ক.opExtendS(origp->অঙ্ক(), origp->অঙ্ক().width());
        } অন্যথা {
            orig_অঙ্ক.opAssign(origp->অঙ্ক());
        }
        V3_সংখ্যা isEq{exprp, 1};
        isEq.opEq(রাশিমালা_অঙ্ক, orig_অঙ্ক);
        ফেরত isEq.isNeqZero();
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
    শূন্য resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (!pinp->exprp()) চালু;
            যদি (!VString::startsWith(pinp->name(), "__VGIfaceParam")) চালু;
            ধ্রুবক Ast_গ্রন্থি_Var_Ref* ধ্রুবক fwdRefp = VN_CAST(pinp->exprp(), গ্রন্থি_Var_Ref);
            যদি (!fwdRefp) চালু;
            ধ্রুবক AstIfaceRefDType* ধ্রুবক resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ধ্রুবক newIrefp = নতুন AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            যদি (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_বৃক্ষ(সত্য));
            }
            pinp->exprp()->unlinkFrBack()->delete_বৃক্ষ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    শূন্য name_By_Pin_মান(AstPin* pinp, Ast_গ্রন্থি_Module* srcModp, AstVar* modvarp, string& longnamer,
                        যৌক্তিক& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_মান_সংখ্যা(pinp->exprp());
        any_overridesr = সত্য;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    শূন্য cellPinCleanup(Ast_গ্রন্থি* nodep, AstPin* pinp, AstPin* paramsp, Ast_গ্রন্থি_Module* srcModp,
                        string& longnamer, যৌক্তিক& any_overridesr,
                        std::unordered_set<ধ্রুবক AstPin*>& overridingTypePinsr) {
        যদি (!pinp->exprp()) ফেরত;  // No-connect
        যদি (AstVar* ধ্রুবক modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            যদি (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } অন্যথা যদি (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_মান(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } অন্যথা {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                যদি (is_Aggregate_Param_মান(pinp->exprp())) {
                    name_By_Pin_মান(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    ফেরত;
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
                যদি (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_আকার))) {
                    ধ্রুবক যৌক্তিক dependent = modvarp->valuep()->exists(
                        [](Ast_গ্রন্থি* np) { ফেরত VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    যদি (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                যদি (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ধ্রুবক exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ধ্রুবক origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                যদি (exprp && !exprp->অঙ্ক().isDouble() && !exprp->অঙ্ক().isString()) {
                    AstVar* cloneVarp = modvarp->clone_বৃক্ষ(মিথ্যা);
                    যৌক্তিক cloneVarpUnresolved = মিথ্যা;
                    যদি (Ast_গ্রন্থি* ধ্রুবক oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_বৃক্ষ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_বৃক্ষ(মিথ্যা));
                    যদি (Ast_গ্রন্থি_DType* ধ্রুবক origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        যদি (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_বৃক্ষ();
                        cloneVarp->childDTypep(origDTypep->clone_বৃক্ষ(মিথ্যা));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr পূর্ণসংখ্যা maxSubstIters = 1000;
                        জন্য (পূর্ণসংখ্যা it = 0; it < maxSubstIters; ++it) {
                            যৌক্তিক any = মিথ্যা;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ধ্রুবক targetp = varrefp->varp();
                                Ast_গ্রন্থি* replacep = nullptr;
                                জন্য (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    যদি (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        যদি (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_বৃক্ষ(মিথ্যা);
                                        }
                                        বিরতি;
                                    }
                                }
                                যদি (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_বৃক্ষ(মিথ্যা);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ধ্রুবক patp = VN_CAST(replacep, Pattern);
                                যদি (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_বৃক্ষ(মিথ্যা));
                                }
                                যদি (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_বৃক্ষ(), varrefp);
                                    any = সত্য;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_গ্রন্থি_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ধ্রুবক ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                যদি (!ptdp) ফেরত;
                                AstPin* overridePinp = nullptr;
                                জন্য (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    যদি (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        বিরতি;
                                    }
                                }
                                Ast_গ্রন্থি_DType* ধ্রুবক substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), গ্রন্থি_DType)
                                                   : ptdp->subDTypep();
                                যদি (substp) toReplace.emplace_back(refp, substp);
                            });
                            জন্য (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ধ্রুবক refp = it->first;
                                refp->replaceWith(it->second->clone_বৃক্ষ(মিথ্যা));
                                VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
                                any = সত্য;
                            }
                            যদি (!any) বিরতি;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_গ্রন্থি* np) {
                            যদি (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = সত্য;
                                ফেরত;
                            }
                            যদি (AstEnumItemRef* ধ্রুবক itemRefp = VN_CAST(np, EnumItemRef)) {
                                যদি (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = সত্য;
                                }
                                ফেরত;
                            }
                            AstRefDType* ধ্রুবক refp = VN_CAST(np, RefDType);
                            যদি (!refp) ফেরত;
                            যদি (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = সত্য;
                            }
                            // A type declared in the template must not be widthed there
                            যদি (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = সত্য;
                            }
                        });
                    }
                    যদি (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        যদি (AstConst* ধ্রুবক widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            যদি (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_বৃক্ষ(), cloneVarp);
                }
                AstConst* ধ্রুবক namingExprp = normedNamep ? normedNamep : exprp;
                যদি (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_গ্রন্থি* ধ্রুবক pinExprp = pinp->exprp();
                    pinExprp->replaceWith(নতুন AstConst{pinp->fileline(), AstConst::Widthed_মান{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_বৃক্ষ(), pinExprp);
                } অন্যথা যদি (origp
                           && (exprp->same_বৃক্ষ(origp)
                               || (exprp->অঙ্ক().width() == origp->অঙ্ক().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } অন্যথা যদি (namingExprp->অঙ্ক().isDouble() || namingExprp->অঙ্ক().isString()
                           || namingExprp->অঙ্ক().isFourState()
                           || namingExprp->অঙ্ক().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_মান_সংখ্যা(namingExprp));
                    any_overridesr = সত্য;
                } অন্যথা {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->অঙ্ক().ascii(মিথ্যা));
                    any_overridesr = সত্য;
                }
                যদি (normedNamep) VL_DO_DANGLING(normedNamep->delete_বৃক্ষ(), normedNamep);
            }
        } অন্যথা যদি (AstParamTypeDType* ধ্রুবক modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_গ্রন্থি_DType* rawTypep = VN_CAST(pinp->exprp(), গ্রন্থি_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            যৌক্তিক skipWidthForTemplateStruct = মিথ্যা;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_গ্রন্থি_DType* ধ্রুবক resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                যদি (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_গ্রন্থি_Module* ধ্রুবক ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    যদি (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = সত্য;
                        V3Stats::add_Stat_যোগ("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                যদি (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_গ্রন্থি_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ধ্রুবক Ast_গ্রন্থি_DType* origp = modvarp->skipRefToNonRefp();
            যদি (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } অন্যথা যদি (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } অন্যথা {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                যদি (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), গ্রন্থি_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                যদি (!modvarp->fwdType().is_গ্রন্থি_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                যদি (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } অন্যথা {
                    যদি (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), গ্রন্থি_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    যদি (AstClassRefDType* ধ্রুবক classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        যদি (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } অন্যথা {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    শূন্য cellInterfaceCleanup(AstPin* pinsp, Ast_গ্রন্থি_Module* srcModp, string& longnamer,
                              যৌক্তিক& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        জন্য (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ধ্রুবক AstVar* ধ্রুবক modvarp = pinp->modVarp();
            যদি (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                চালু;
            }
            যদি (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ধ্রুবক portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ধ্রুবক Ast_গ্রন্থি* ধ্রুবক exprp = pinp->exprp();
                যদি (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    চালু;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                যদি (ধ্রুবক Ast_গ্রন্থি_Var_Ref* ধ্রুবক vrp = VN_CAST(exprp, গ্রন্থি_Var_Ref)) {
                    যদি (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                যদি (!pinIrefp && exprp) {
                    ধ্রুবক Ast_গ্রন্থি* basep = exprp;
                    যখন (ধ্রুবক Ast_গ্রন্থি_Pre_Sel* ধ্রুবক selp = VN_CAST(basep, গ্রন্থি_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    যদি (ধ্রুবক AstVarRef* ধ্রুবক vrp = VN_CAST(basep, VarRef)) {
                        যদি (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                যদি (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } অন্যথা যদি (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } অন্যথা {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    যদি (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_মান_সংখ্যা(pinIrefp));
                        any_overridesr = সত্য;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        যদি (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_গ্রন্থি::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_গ্রন্থি::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    শূন্য storeOriginalParams(Ast_গ্রন্থি_Module* ধ্রুবক classp) {
        জন্য (Ast_গ্রন্থি* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_গ্রন্থি* originalParamp = nullptr;
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isGParam()) originalParamp = varp->clonep();
            } অন্যথা যদি (AstParamTypeDType* ধ্রুবক ptp = VN_CAST(stmtp, ParamTypeDType)) {
                যদি (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            যদি (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    শূন্য genericInterfaceVarSetup(ধ্রুবক AstPin* ধ্রুবক paramsp, ধ্রুবক AstPin* ধ্রুবক pinsp) {
        std::unordered_map<string, ধ্রুবক AstPin*> paramspMap;
        জন্য (ধ্রুবক AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ঢোকাও({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        যদি (paramspMap.empty()) ফেরত;

        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstPin* ধ্রুবক pinp = VN_CAST(nodep, Pin)) {
                যদি (AstVar* ধ্রুবক varp = pinp->modVarp()) {
                    Ast_গ্রন্থি_DType* dtypep = varp->childDTypep()->elemDTypep();
                    যদি (AstIfaceGenericDType* ধ্রুবক ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ধ্রুবক auto iter = paramspMap.find(varp->name());
                        যদি (iter == paramspMap.end()) চালু;
                        Ast_গ্রন্থি* ধ্রুবক backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ধ্রুবক AstPin* ধ্রুবক paramp = iter->second;
                        paramspMap.erase(iter);
                        ধ্রুবক AstIfaceRefDType* ধ্রুবক ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ধ্রুবক newIfacerefp = নতুন AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        যদি (auto* ধ্রুবক arrDtp = VN_CAST(backp, গ্রন্থি_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } অন্যথা যদি (auto* ধ্রুবক arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } অন্যথা {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ঠেলো_Deletep(ifaceGDTypep), ifaceGDTypep);
                        যদি (paramspMap.empty()) ফেরত;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    শূন্য resolveDefaultParams(Ast_গ্রন্থি* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        যদি (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            যদি (!classp) ফেরত;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } অন্যথা যদি (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } অন্যথা {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_সূচক.clear();
        জন্য (Ast_গ্রন্থি* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                যদি (!paramTypep->isGParam()) চালু;
                m_param_সূচক.emplace(paramTypep, static_cast<পূর্ণসংখ্যা>(m_classTypeParams.আকার_2()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_গ্রন্থি_DType*> defaultTypeNodes(m_classTypeParams.আকার_2(), nullptr);
        জন্য (আকার_t i = 0; i < m_classTypeParams.আকার_2(); ++i) {
            AstParamTypeDType* ধ্রুবক paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ধ্রুবক reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            যদি (!reqDtp) চালু;

            // If default is a reference to another param type, record dependency
            যদি (AstRefDType* ধ্রুবক refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                যদি (AstParamTypeDType* ধ্রুবক sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_param_সূচক.find(sourceParamp);
                    যদি (it != m_param_সূচক.end()) { m_classTypeParams[i].second = it->second; }
                    চালু;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtp = VN_CAST(reqDtp->lhsp(), গ্রন্থি_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pins_By_সূচক;
        pins_By_সূচক.resize(m_classTypeParams.আকার_2(), nullptr);
        জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ধ্রুবক typep = pinp->modPTypep();
            ধ্রুবক auto it = typep ? m_param_সূচক.find(typep) : m_param_সূচক.end();
            যদি (it != m_param_সূচক.end()) {
                pins_By_সূচক[it->second] = pinp;
            } অন্যথা {
                pins_By_সূচক.ঠেলো_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        জন্য (যৌক্তিক resolvedSome = সত্য; resolvedSome;) {
            resolvedSome = মিথ্যা;
            জন্য (আকার_t paramIdx = 0; paramIdx < m_classTypeParams.আকার_2(); paramIdx++) {
                যদি (pins_By_সূচক[paramIdx]) চালু;
                ধ্রুবক পূর্ণসংখ্যা sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                যদি (sourceParamIdx >= 0 && pins_By_সূচক[sourceParamIdx]) {
                    newPinp = pins_By_সূচক[sourceParamIdx]->clone_বৃক্ষ(মিথ্যা);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                যদি (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_গ্রন্থি_DType* ধ্রুবক dtypep = defaultTypeNodes[paramIdx];
                    newPinp = নতুন AstPin{dtypep->fileline(), static_cast<পূর্ণসংখ্যা>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_বৃক্ষ(মিথ্যা)};
                }

                যদি (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(সত্য);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    যদি (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } অন্যথা যদি (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_সূচক[paramIdx] = newPinp;
                    যদি (!paramsp) paramsp = newPinp;
                    resolvedSome = সত্য;
                }
            }
        }
    }

    Ast_গ্রন্থি_Module* গ্রন্থি_Deparam_Common(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, যৌক্তিক any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        যদি (debug() >= 9 && paramsp) paramsp->dump_বৃক্ষ_And_পরবর্তী(cout, "-  cellparams: ");

        যদি (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } অন্যথা {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ধ্রুবক AstPin*> overridingTypePins;
            জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (!pinp->exprp()) চালু;  // An empty override keeps the default
                যদি (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    চালু;
                }
                AstVar* ধ্রুবক modvarp = pinp->modVarp();
                যদি (!modvarp || !modvarp->isGParam()) চালু;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ধ্রুবক AstPattern* ধ্রুবক patternp = VN_CAST(pinp->exprp(), Pattern);
                যদি (patternp && !patternp->childDTypep()) চালু;
                যদি (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_মান(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (AstParamTypeDType* ধ্রুবক modptp = pinp->modPTypep()) {
                    যদি (!overridingTypePins.গুনতি(pinp)) চালু;
                    Ast_গ্রন্থি_DType* ধ্রুবক typep = VN_AS(pinp->exprp(), গ্রন্থি_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_মান_সংখ্যা(typep->skipRefToNonRefp());
                    any_overrides = সত্য;
                    চালু;
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
        যৌক্তিক defaultsResolved = মিথ্যা;
        যদি (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            জন্য (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                যদি (pinp->modPTypep()) {
                    any_overrides = সত্য;
                    defaultsResolved = সত্য;
                    বিরতি;
                }
            }
        }
        যদি (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = সত্য;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_গ্রন্থি_Module* newModp = nullptr;
        যদি (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_গ্রন্থি_Module* ধ্রুবক paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            যদি (!paramedModp) ফেরত nullptr;  // Unsupported, already reported
            paramedModp->dead(মিথ্যা);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } অন্যথা যদি (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            যদি (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_গ্রন্থি_Module* গ্রন্থি_Copyp = srcModp->clone_বৃক্ষ(মিথ্যা);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << গ্রন্থি_Copyp);
                m_deleter.ঠেলো_Deletep(গ্রন্থি_Copyp);  // nodeCopyp used past here
                srcModp->user3p(গ্রন্থি_Copyp);
                storeOriginalParams(গ্রন্থি_Copyp);
            }
            newModp = srcModp;
        } অন্যথা {
            ধ্রুবক string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ধ্রুবক ModInfo* ধ্রুবক modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            যদি (!modInfop) ফেরত nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ধ্রুবক যৌক্তিক cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ধ্রুবক newClassp = VN_CAST(newModp, Class);
        যদি (newClassp && !newClassp->user2()) m_specializedClassps.ঠেলো_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        যদি (defaultsResolved) srcModp->user4p(newModp);

        জন্য (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                যদি (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        যদি (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ(), paramsp);
        ফেরত newModp;
    }

    Ast_গ্রন্থি_Module* cellDeparam(AstCell* nodep, Ast_গ্রন্থি_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_গ্রন্থি_Module* ধ্রুবক newModp = গ্রন্থি_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        যদি (!newModp) ফেরত nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(মিথ্যা);
        ফেরত newModp;
    }
    Ast_গ্রন্থি_Module* ifaceRefDeparam(AstIfaceRefDType* ধ্রুবক nodep, Ast_গ্রন্থি_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        যদি (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ধ্রুবক string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ধ্রুবক string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ধ্রুবক আকার_t ownerPos = ownerBaseName.find("__");
            যদি (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ধ্রুবক আকার_t srcPos = srcBaseName.find("__");
            যদি (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            যদি (ownerBaseName == srcBaseName) {
                যৌক্তিক allOwnParams = সত্য;
                জন্য (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    যদি (AstRefDType* ধ্রুবক refp = VN_CAST(pinp->exprp(), RefDType)) {
                        যদি (AstParamTypeDType* ধ্রুবক ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_গ্রন্থি_Module* ধ্রুবক ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            যদি (ptdOwnerp != m_modp) allOwnParams = মিথ্যা;
                        } অন্যথা {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } অন্যথা {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                যদি (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::add_Stat_যোগ("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    যদি (nodep->paramsp()) nodep->paramsp()->unlink_Fr_Back_With_পরবর্তী()->delete_বৃক্ষ();
                    ফেরত ownerIfacep;
                }
            }
        }

        Ast_গ্রন্থি_Module* ধ্রুবক newModp
            = গ্রন্থি_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, মিথ্যা);
        যদি (!newModp) ফেরত nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        ফেরত newModp;
    }
    Ast_গ্রন্থি_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_গ্রন্থি_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_গ্রন্থি_Module* ধ্রুবক newModp
            = গ্রন্থি_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, মিথ্যা);
        যদি (!newModp) ফেরত nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ধ্রুবক refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ধ্রুবক newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        যদি (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            যদি (AstTypedef* ধ্রুবক typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        ফেরত newModp;
    }
    Ast_গ্রন্থি_Module* classRefDeparam(AstClassRefDType* nodep, Ast_গ্রন্থি_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_গ্রন্থি_Module* ধ্রুবক newModp
            = গ্রন্থি_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, মিথ্যা);
        যদি (!newModp) ফেরত nullptr;
        AstClass* ধ্রুবক newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        ফেরত newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    শ্রেণী DeferredResolverVisitor final : সর্বজনীন VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ধ্রুবক Ast_গ্রন্থি*> m_reachedDecls;

        যৌক্তিক firstReach(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক declp) { ফেরত m_reachedDecls.ঢোকাও(declp).second; }

        শূন্য iterateParamType(AstParamTypeDType* ধ্রুবক paramTypep) {
            যদি (firstReach(paramTypep)) iterate(paramTypep);
        }
        শূন্য iterateTypedef(AstTypedef* ধ্রুবক tdefp) {
            যদি (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
        শূন্য visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_গ্রন্থি* ধ্রুবক targetp = nodep->classOrPackageNodep();
            যদি (AstTypedef* ধ্রুবক tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } অন্যথা যদি (AstParamTypeDType* ধ্রুবক paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        শূন্য visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        শূন্য visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ধ্রুবক tdefp = nodep->typedefp();
            যদি (tdefp) {
                iterateTypedef(tdefp);
            } অন্যথা যদি (AstParamTypeDType* ধ্রুবক paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        শূন্য visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ধ্রুবক varp = nodep->varp();
            যদি (firstReach(varp)) {
                iterate(varp->subDTypep());
                ধ্রুবক auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                যদি (varp->varType() == VVarType::LPARAM && deferredVarps.গুনতি(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    সর্বজনীন:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_গ্রন্থি* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

সর্বজনীন:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    শূন্য retargetIfaceRefs(Ast_গ্রন্থি_Module* parentModp, ধ্রুবক string& cellName) {
        Ast_গ্রন্থি_Module* ধ্রুবক correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        যদি (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) ফেরত;
        parentModp->foreach([&](AstRefDType* refp) {
            ধ্রুবক VIfaceCaptureTag* ধ্রুবক tagp = V3LinkDotIfaceCapture::captureTag(refp);
            যদি (!tagp || tagp->m_cellPath != cellName) ফেরত;
            // Only retarget REFDTYPEs owned by parentModp.
            যদি (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) ফেরত;
            যদি (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    শূন্য resolveDeferredDotsReachableFrom(Ast_গ্রন্থি* rootp, ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*এই, rootp};
    }

    Ast_গ্রন্থি_Module* গ্রন্থি_Deparam(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_Module* srcModp, Ast_গ্রন্থি_Module* modp,
                               ধ্রুবক string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_গ্রন্থি::গ্রন্থি_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string গ্রন্থি_Name = nodep->name();
        যদি (AstIfaceRefDType* ধ্রুবক ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            যদি (গ্রন্থি_Name.empty()) গ্রন্থি_Name = ifaceRefp->cellName();
        }
        ধ্রুবক string instanceName
            = গ্রন্থি_Name.empty() ? someInstanceName : (someInstanceName + "." + গ্রন্থি_Name);

        যদি (AstCell* ধ্রুবক cellp = VN_CAST(nodep, Cell)) {
            জন্য (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ধ্রুবক nextp = VN_AS(pinp->nextp(), Pin);
                যদি (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_বৃক্ষ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_গ্রন্থি::গ্রন্থি_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_গ্রন্থি_Module* newModp = nullptr;
        যদি (AstCell* ধ্রুবক cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } অন্যথা যদি (AstIfaceRefDType* ধ্রুবক ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } অন্যথা যদি (AstClassRefDType* ধ্রুবক classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } অন্যথা যদি (AstClassOrPackageRef* ধ্রুবক classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } অন্যথা {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        যদি (!newModp) ফেরত nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_গ্রন্থি::গ্রন্থি_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        ফেরত newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        ফেরত taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        জন্য (Ast_গ্রন্থি_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
            m_allModuleNames.ঢোকাও(modp->name());
        }
    }
    ~ParamProcessor() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

শ্রেণী ClassRefUnlinkerVisitor final : সর্বজনীন VNVisitor {

সর্বজনীন:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        যদি (nodep->paramsp()) {
            যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                যদি (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ধ্রুবক dotp = VN_CAST(nodep->backp(), Dot);
                    যদি (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(ঠেলো_Deletep(dotp), dotp, nodep);
                    } অন্যথা {
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    শূন্য visit(AstClass* nodep) override {}  // don't iterate inside classes
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

শ্রেণী ParamClassRefDTypeRelinkVisitor final : সর্বজনীন VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ধ্রুবক AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_গ্রন্থি_Module* m_ownerModp = nullptr;
    Ast_গ্রন্থি_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    শূন্য ensureOwnerMap() {
        যদি (m_ownerMapForp == m_ownerModp) ফেরত;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        যদি (!m_ownerModp) ফেরত;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        যদি (AstClass* ধ্রুবক ownerClassp = VN_CAST(m_ownerModp, Class)) {
            যদি (ownerClassp->hasGParam()) ফেরত;
        }
        জন্য (Ast_গ্রন্থি* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ধ্রুবক tdp = VN_CAST(sp, Typedef);
            যদি (!tdp) চালু;
            AstClassRefDType* ধ্রুবক crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            যদি (!crdtp || !crdtp->classp()) চালু;
            AstClass* ধ্রুবক targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            যদি (targetp->hasGParam()) চালু;
            ধ্রুবক std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ধ্রুবক auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            যদি (!pair.second && pair.first->second != targetp) m_ambiguous.ঢোকাও(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    স্থির string classOrigName(ধ্রুবক AstClass* classp) {
        ফেরত classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    স্থির AstTypedef* findTypedefWithBases(AstClass* classp, ধ্রুবক string& name) {
        জন্য (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            যদি (AstTypedef* ধ্রুবক tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                যদি (tdp->subDTypep()) ফেরত tdp;
            }
        }
        ফেরত nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    শূন্য retargetRefDType(AstRefDType* refp) {
        যদি (!m_ownerModp) ফেরত;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        যদি (AstClass* ধ্রুবক ownerClassp = VN_CAST(m_ownerModp, Class)) {
            যদি (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ধ্রুবক AstClass* refClassp = nullptr;
                যদি (AstTypedef* ধ্রুবক tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } অন্যথা যদি (ধ্রুবক AstClassOrPackageRef* ধ্রুবক classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                যদি (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ধ্রুবক selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    যদি (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        ফেরত;
                    }
                }
            }
        }

        AstTypedef* ধ্রুবক oldTdp = refp->typedefp();
        যদি (!oldTdp) ফেরত;
        AstClass* ধ্রুবক oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        যদি (!oldOwnerp) ফেরত;

        ensureOwnerMap();
        যদি (m_origNameToClone.empty()) ফেরত;
        ধ্রুবক std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        যদি (m_ambiguous.গুনতি(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            ফেরত;
        }
        ধ্রুবক auto it = m_origNameToClone.find(origName);
        যদি (it == m_origNameToClone.end()) ফেরত;
        AstClass* ধ্রুবক correctClassp = it->second;
        যদি (correctClassp == oldOwnerp) ফেরত;
        AstTypedef* ধ্রুবক newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        যদি (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            ফেরত;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        যদি (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            ফেরত;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

সর্বজনীন:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        জন্য (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstParamTypeDType* ধ্রুবক ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        যদি (AstParamTypeDType* ধ্রুবক paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ধ্রুবক auto it = m_paramTypeClassMap.find(paramtypep);
            যদি (it != m_paramTypeClassMap.end()) {
                AstClass* ধ্রুবক origClassp = it->second;
                যদি (origClassp->hasGParam() && !origClassp->user3p()) {
                    যদি (AstClass* ধ্রুবক parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ধ্রুবক string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        জন্য (Ast_গ্রন্থি* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            যদি (AstParamTypeDType* ধ্রুবক ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                যদি (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    বিরতি;
                                }
                            }
                        }
                        যদি (newParamTypep) {
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
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

শ্রেণী ParamState final {
সর্বজনীন:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<যৌক্তিক, পূর্ণসংখ্যা>;
    std::multimap<WQKey, Ast_গ্রন্থি_Module*> m_work_Queue_পরবর্তী;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_গ্রন্থি_Module*, std::unordered_set<Ast_গ্রন্থি_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

শ্রেণী ParamVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    যৌক্তিক m_iterateModule = মিথ্যা;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<যৌক্তিক, Ast_গ্রন্থি*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ধ্রুবক AstRefDType*, যৌক্তিক>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    যৌক্তিক m_modIfaceRefsDone = মিথ্যা;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    যৌক্তিক m_inGenerateCond = মিথ্যা;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    শূন্য processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_গ্রন্থি_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = সত্য;

        // Visit all cells under module, recursively
        যখন (সত্য) {
            // Classes specialized since the last pass still need their bodies elaborated
            জন্য (AstClass* ধ্রুবক classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_Queue_পরবর্তী.emplace(ParamState::WQKey{সত্য, classp->level()}, classp);
            }
            যদি (workQueue.empty()) std::swap(workQueue, m_state.m_work_Queue_পরবর্তী);
            যদি (workQueue.empty()) বিরতি;

            ধ্রুবক auto itm = workQueue.cbegin();
            Ast_গ্রন্থি_Module* ধ্রুবক modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            যদি (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                যদি (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = মিথ্যা;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            যখন (!m_cellps.empty()) {
                ধ্রুবক auto itim = m_cellps.cbegin();
                Ast_গ্রন্থি* ধ্রুবক cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ধ্রুবক auto genHierIt = m_genHierNames.find(cellp);
                যদি (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_গ্রন্থি_Module* srcModp = nullptr;
                যদি (ধ্রুবক AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } অন্যথা যদি (ধ্রুবক AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    যদি (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) চালু;
                } অন্যথা যদি (ধ্রুবক AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } অন্যথা যদি (ধ্রুবক AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } অন্যথা {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                যদি (!srcModp) চালু;

                // Update path
                ধ্রুবক string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                যদি (Ast_গ্রন্থি_Module* ধ্রুবক newModp
                    = m_processor.গ্রন্থি_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    যদি (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        যদি (V3LinkDotIfaceCapture::enabled()) {
                            যদি (ধ্রুবক AstCell* ধ্রুবক modCellp = VN_CAST(cellp, Cell)) {
                                যদি (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ঢোকাও(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    যদি (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = মিথ্যা;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    স্থির string getRefBaseName(ধ্রুবক AstVarXRef* refp) {
        ধ্রুবক string dotted = refp->dotted();
        যদি (dotted.empty()) ফেরত "";
        ফেরত dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    শূন্য logTemplateLeakRefs(Ast_গ্রন্থি_Module* parentModp, Ast_গ্রন্থি_Module* templateModp,
                             ধ্রুবক অক্ষর* stage, Ast_গ্রন্থি* contextp) {
        যদি (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) ফেরত;
        // LCOV_EXCL_START  // Debug-only diagnostic
        পূর্ণসংখ্যা leak_গুনতি = 0;
        ধ্রুবক auto ancestryOf = [](ধ্রুবক Ast_গ্রন্থি* nodep) {
            string ancestry;
            জন্য (ধ্রুবক Ast_গ্রন্থি* curp = nodep; curp; curp = curp->backp()) {
                যদি (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                যদি (VN_IS(curp, গ্রন্থি_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    বিরতি;
                }
            }
            ফেরত ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            যদি (refp->typedefp()) {
                Ast_গ্রন্থি_Module* ধ্রুবক tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                যদি (tdOwnerp == templateModp) {
                    ++leak_গুনতি;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_গ্রন্থি::গ্রন্থি_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            যদি (refp->refDTypep()) {
                Ast_গ্রন্থি_Module* ধ্রুবক rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                যদি (rdOwnerp == templateModp) {
                    ++leak_গুনতি;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_গ্রন্থি::গ্রন্থি_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            যদি (!varrefp->varp()) ফেরত;
            Ast_গ্রন্থি_Module* ধ্রুবক varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            যদি (varOwnerp != templateModp) ফেরত;
            ++leak_গুনতি;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_গ্রন্থি::গ্রন্থি_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        যদি (leak_গুনতি > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_গুনতি);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    শূন্য checkParamNotHierRecurse(Ast_গ্রন্থি* nodep, যৌক্তিক underTypeQuery = মিথ্যা) {
        জন্য (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ধ্রুবক AstAttrOf* ধ্রুবক attrp = VN_CAST(nodep, AttrOf);
            ধ্রুবক যৌক্তিক childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            যদি (ধ্রুবক AstVarXRef* ধ্রুবক refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                যৌক্তিক isIfaceRef = মিথ্যা;
                যদি (refp->varp() && refp->varp()->isIfaceParam()) {
                    ধ্রুবক string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.গুনতি(refname) || m_ifaceInstCells.গুনতি(refname));
                }
                যদি (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_FTask_Ref* ধ্রুবক refp = VN_CAST(nodep, গ্রন্থি_FTask_Ref)) {
                যদি (refp->dotted() != "") {
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
    শূন্য specializeNestedIfaceCells(Ast_গ্রন্থি_Module* ifaceModp) {
        জন্য (Ast_গ্রন্থি* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ধ্রুবক nestedCellp = VN_CAST(stmtp, Cell);
            যদি (!nestedCellp) চালু;
            যদি (!VN_IS(nestedCellp->modp(), Iface)) চালু;
            যদি (!nestedCellp->paramsp()) চালু;

            Ast_গ্রন্থি_Module* ধ্রুবক nestedSrcModp = nestedCellp->modp();
            যদি (Ast_গ্রন্থি_Module* ধ্রুবক nestedNewModp = m_processor.গ্রন্থি_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                যদি (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    জন্য (Ast_গ্রন্থি* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        যদি (AstVar* ধ্রুবক varp = VN_CAST(sp, Var)) {
                            যদি (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    যদি (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    যৌক্তিক cellParamsReferenceIfacePorts(AstCell* cellp) {
        যদি (!cellp->paramsp()) ফেরত মিথ্যা;

        জন্য (Ast_গ্রন্থি* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            যদি (AstPin* ধ্রুবক pinp = VN_CAST(nodep, Pin)) {
                যদি (Ast_গ্রন্থি* ধ্রুবক exprp = pinp->exprp()) {
                    যদি (ধ্রুবক AstVarXRef* ধ্রুবক refp = VN_CAST(exprp, VarXRef)) {
                        ধ্রুবক string refname = getRefBaseName(refp);
                        যদি (!refname.empty()
                            && (m_ifacePortNames.গুনতি(refname)
                                || m_ifaceInstCells.গুনতি(refname)))
                            ফেরত সত্য;
                    }
                }
            }
        }
        ফেরত মিথ্যা;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    শূন্য expandIfaceArrayPortDimensions(AstVar* portp, Ast_গ্রন্থি_DType* dtypep,
                                        ধ্রুবক std::string& suffix) {
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        যদি (!arrp) {
            AstVar* ধ্রুবক varp = portp->clone_বৃক্ষ(মিথ্যা);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            যদি (Ast_গ্রন্থি_DType* ধ্রুবক oldp = varp->childDTypep()) {
                VL_DO_DANGLING(ঠেলো_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_বৃক্ষ(মিথ্যা));
            portp->add_পরবর্তী_Here(varp);
            ফেরত;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        জন্য (ধ্রুবক পূর্ণসংখ্যা n : arrp->declRange().seq_ডান_To_বাম()) {
            ধ্রুবক std::string s = suffix + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    শূন্য expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ধ্রুবক AstRange* rangep, ধ্রুবক std::string& suffix, পূর্ণসংখ্যা idx) {
        // Base case: insert the element when no dimensions left
        যদি (!rangep) {
            AstCell* ধ্রুবক elemp = arrayedCellp->clone_বৃক্ষ(মিথ্যা);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->add_পরবর্তী_Here(elemp);
            যদি (ifaceVarp) {
                AstIfaceRefDType* ধ্রুবক irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_বৃক্ষ(মিথ্যা);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ধ্রুবক varp = ifaceVarp->clone_বৃক্ষ(মিথ্যা);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                যদি (Ast_গ্রন্থি_DType* ধ্রুবক oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(ঠেলো_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->add_পরবর্তী_Here(varp);
            }
            ফেরত;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ধ্রুবক AstRange* ধ্রুবক subRangep = VN_AS(rangep->nextp(), Range);
        জন্য (ধ্রুবক পূর্ণসংখ্যা n : rangep->seq_ডান_To_বাম()) {
            ধ্রুবক std::string s = suffix + "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    শূন্য visitCellOrClassRef(Ast_গ্রন্থি* nodep, যৌক্তিক isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        যদি (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ধ্রুবক cellp = VN_CAST(nodep, Cell);
            যদি (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_গ্রন্থি_Module* ধ্রুবক srcModp = cellp->modp();
                Ast_গ্রন্থি_Module* ধ্রুবক newModp
                    = m_processor.গ্রন্থি_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                যদি (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                যদি (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        যদি (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->recursiveClone()) nodep->dead(সত্য);  // Fake, made for recursive elimination
        যদি (nodep->dead()) ফেরত;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) {
            যদি (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ঠেলো_back(classp);
                ফেরত;
            }
        }

        যদি (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_Queue_পরবর্তী.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            ফেরত;
        }

        // Start traversal at root-like things
        যদি (nodep->is_শীর্ষ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_Queue_পরবর্তী.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    যৌক্তিক isCircularType(ধ্রুবক AstRefDType* nodep) {
        ধ্রুবক auto iter = m_isCircular.emplace(nodep, সত্য);
        যদি (!iter.second) ফেরত iter.first->second;
        যদি (ধ্রুবক AstRefDType* ধ্রুবক subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ধ্রুবক যৌক্তিক ret = isCircularType(subDTypep);
            iter.first->second = ret;
            ফেরত ret;
        }
        iter.first->second = মিথ্যা;
        ফেরত মিথ্যা;
    }

    শূন্য visit(AstRefDType* nodep) override {
        ধ্রুবক যৌক্তিক isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        যদি (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        যদি (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } অন্যথা যদি (nodep->typedefp() && nodep->subDTypep()
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
    শূন্য visit(AstCell* nodep) override {
        যদি (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            জন্য (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ধ্রুবক ifaceVarp = [&]() -> AstVar* {
                যদি (!VN_IS(nodep->modp(), Iface)) ফেরত nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ধ্রুবক varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                ফেরত varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            যদি (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(সত্য);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ধ্রুবক firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ধ্রুবক irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            ফেরত;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        যদি (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    শূন্য visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        যদি (nodep->ifacep()) visitCellOrClassRef(nodep, সত্য);
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, মিথ্যা);
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        যদি (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, মিথ্যা);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    স্থির শূন্য gatherVars(Ast_গ্রন্থি* stmtsp, যৌক্তিক (*matchp)(ধ্রুবক AstVar*), VarsByName& varsr) {
        জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                যদি (matchp(varp)) varsr.emplace(varp->name(), varp);
            } অন্যথা যদি (AstGenBlock* ধ্রুবক genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ধ্রুবক VarsByName& modIfaceRefs() {
        যদি (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ধ্রুবক AstVar* varp) { ফেরত varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = সত্য;
        }
        ফেরত m_modIfaceRefs;
    }
    ধ্রুবক VarsByName& ifaceParams(Ast_গ্রন্থি_Module* ifacep) {
        ধ্রুবক auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        যদি (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ধ্রুবক AstVar* varp) { ফেরত varp->isParam(); },
                pair.first->second);
        }
        ফেরত pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_গ্রন্থি* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ধ্রুবক lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ধ্রুবক rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        যদি (!lhsp || !rhsp) ফেরত nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        যদি (VL_UNCOVERABLE(!m_modp)) ফেরত nullptr;

        ধ্রুবক auto& refVars = modIfaceRefs();
        ধ্রুবক auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        যদি (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) ফেরত nullptr;
        AstVar* ধ্রুবক ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ধ্রুবক ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        যদি (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) ফেরত nullptr;

        ধ্রুবক auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ধ্রুবক auto targetVarIt = ifaceVars.find(rhsp->name());
        যদি (targetVarIt == ifaceVars.end()) ফেরত nullptr;
        AstVar* ধ্রুবক targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        যদি (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        যদি (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            ফেরত nullptr;  // LCOV_EXCL_LINE
        }
        ফেরত targetVarp->valuep()->clone_বৃক্ষ(মিথ্যা);
    }
    শূন্য resolveGenericIfaceDotsIn(Ast_গ্রন্থি* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ঠেলো_back(dotp); });
        জন্য (AstDot* ধ্রুবক dotp : dotps) {
            যদি (Ast_গ্রন্থি* ধ্রুবক newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    শূন্য visit(AstVar* nodep) override {
        যদি (nodep->user2SetOnce()) ফেরত;  // Process once
        // Interface array port: split it into its elements
        যদি (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(সত্য);
        }
        // Build cache of interface port names as we encounter them
        যদি (nodep->isIfaceRef()) { m_ifacePortNames.ঢোকাও(nodep->name()); }
        iterateChildren(nodep);
        যদি (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            যদি (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } অন্যথা যদি (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                যৌক্তিক hasUnresolvedLparamXRef = মিথ্যা;
                nodep->valuep()->foreach([&](ধ্রুবক AstVarXRef* xrefp) {
                    যদি (ধ্রুবক AstVar* ধ্রুবক varp = xrefp->varp()) {
                        যদি (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = সত্য;
                        }
                    }
                });
                যদি (hasUnresolvedLparamXRef) ফেরত;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ধ্রুবক যৌক্তিক hasDot = nodep->valuep()->exists([](AstDot*) { ফেরত সত্য; });
                যৌক্তিক refsDeferred = মিথ্যা;
                যদি (!hasDot) {
                    ধ্রুবক auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ধ্রুবক AstVarRef* refp) {
                        যদি (refsDeferred) ফেরত;
                        AstVar* ধ্রুবক refVarp = refp->varp();
                        যদি (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.গুনতি(refVarp)) {
                            refsDeferred = সত্য;
                        }
                    });
                }
                যদি (hasDot || refsDeferred) {
                    v3Global.rootp()->ঠেলো_Deferred_Param_Varp(nodep);
                    ফেরত;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        যদি (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    শূন্য visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        যদি (nodep->varp()) iterate(nodep->varp());
    }
    যৌক্তিক ifaceParamReplace(AstVarXRef* nodep, Ast_গ্রন্থি* candp) {
        জন্য (; candp; candp = candp->nextp()) {
            যদি (nodep->name() == candp->name()) {
                যদি (AstVar* ধ্রুবক varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    যদি (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    ফেরত সত্য;
                } অন্যথা যদি (ধ্রুবক AstPin* ধ্রুবক pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_বৃক্ষ(মিথ্যা));
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ফেরত সত্য;
                }
            }
        }
        ফেরত মিথ্যা;
    }

    শূন্য deparamIfaceCellNow(AstCell* cellp) {
        যদি (!cellp->paramsp()) ফেরত;
        যদি (!VN_IS(cellp->modp(), Iface)) ফেরত;
        Ast_গ্রন্থি_Module* ধ্রুবক srcModp = cellp->modp();
        Ast_গ্রন্থি_Module* ধ্রুবক newModp
            = m_processor.গ্রন্থি_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        যদি (newModp && newModp != srcModp) {
            যদি (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            ফেরত;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        যদি (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarXRef* nodep) override {
        যদি (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            ফেরত;
        }
        // Parameters of an interface array are accessed through its first element
        যদি (nodep->varp() && nodep->varp()->isParam()) {
            ধ্রুবক auto it = m_ifaceInstCells.find(nodep->dotted());
            যদি (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ধ্রুবক string dotted = nodep->dotted();
        যদি (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ধ্রুবক Ast_গ্রন্থি* backp = nodep;
            যখন ((backp = backp->backp())) {
                যদি (VN_IS(backp, গ্রন্থি_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    বিরতি;
                }
                যদি (ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(backp, Var)) {
                    যদি (!varp->isIfaceRef()) চালু;
                    // Through all dimensions of an array
                    ধ্রুবক AstIfaceRefDType* ধ্রুবক ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    যদি (!ifacerefp) চালু;
                    // Interfaces passed in on the port map have ifaces
                    যদি (ধ্রুবক AstIface* ধ্রুবক ifacep = ifacerefp->ifacep()) {
                        যদি (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            যদি (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                ফেরত;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    অন্যথা যদি (AstCell* ধ্রুবক cellp = ifacerefp->cellp()) {
                        যদি (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            যদি (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                ফেরত;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ধ্রুবক auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            যদি (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ধ্রুবক cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    শূন্য visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ধ্রুবক AstClass* lhsClassp = nullptr;
        ধ্রুবক AstClassOrPackageRef* ধ্রুবক classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        যদি (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_গ্রন্থি* rhsDefp = nullptr;
        AstClassOrPackageRef* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        যদি (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        যদি (lhsClassp && rhsDefp) {
            m_state.m_dots.ঠেলো_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } অন্যথা {
            iterate(nodep->rhsp());
        }
    }

    শূন্য visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ধ্রুবক varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_গ্রন্থি_FTask_Ref* ধ্রুবক taskrefp = VN_CAST(nodep->refp(), গ্রন্থি_FTask_Ref);
        AstEnumItemRef* ধ্রুবক enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        যদি (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } অন্যথা যদি (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } অন্যথা যদি (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } অন্যথা {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            ফেরত;
        }
        iterate(nodep->cellrefp());

        যদি (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } অন্যথা যদি (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } অন্যথা {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        জন্য (Ast_গ্রন্থি *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<পূর্ণসংখ্যা> indices;
        জন্য (Ast_গ্রন্থি* s = nodep->selp(); s; s = s->nextp()) {
            ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(s, Const);
            যদি (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                ফেরত;
            }
            indices.ঠেলো_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ধ্রুবক string viftopSuffix = "__Viftop";
        ধ্রুবক string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().আকার_2() - viftopSuffix.আকার_2())
                  : nodep->name();
        ধ্রুবক string placeholder = "__BRA__??__KET__";
        আকার_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        যদি (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            ফেরত;
        }
        pos += baseName.দৈর্ঘ্য();
        জন্য (পূর্ণসংখ্যা idx : indices) {
            ধ্রুবক string replacement = "__BRA__" + Ast_গ্রন্থি::encode_সংখ্যা(idx) + "__KET__";
            যদি (m_unlinkedTxt.compare(pos, placeholder.দৈর্ঘ্য(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                ফেরত;
            }
            m_unlinkedTxt.replace(pos, placeholder.দৈর্ঘ্য(), replacement);
            pos += replacement.দৈর্ঘ্য();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    শূন্য iterateGenerateCond(Ast_গ্রন্থি* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = সত্য;
        iterate_And_পরবর্তী_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    শূন্য visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->condp(), Const)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_Back_With_পরবর্তী();
                nodep->replaceWith(keepp);
            } অন্যথা {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            // Normal edit rules will now recurse the replacement
        } অন্যথা {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    শূন্য visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        যদি (AstGenFor* ধ্রুবক forp = VN_AS(nodep->genforp(), GenFor)) {
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
            ধ্রুবক string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_Back_With_পরবর্তী();
                nodep->add_পরবর্তী_Here(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } অন্যথা {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    শূন্য visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        যৌক্তিক hit = মিথ্যা;
        Ast_গ্রন্থি* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        জন্য (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            জন্য (Ast_গ্রন্থি* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ধ্রুবক AstConst* ধ্রুবক exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        জন্য (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            জন্য (Ast_গ্রন্থি* ep = itemp->condsp(); ep;) {
                Ast_গ্রন্থি* ধ্রুবক nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        জন্য (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            যদি (!itemp->isDefault()) {
                জন্য (Ast_গ্রন্থি* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    যদি (ধ্রুবক AstConst* ধ্রুবক ccondp = VN_CAST(ep, Const)) {
                        V3_সংখ্যা match{nodep, 1};
                        match.opEq(ccondp->অঙ্ক(), exprp->অঙ্ক());
                        যদি (!hit && match.isNeqZero()) {
                            hit = সত্য;
                            keepp = itemp->itemsp();
                        }
                    } অন্যথা {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        জন্য (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            যদি (itemp->isDefault()) {
                যদি (!hit) {
                    hit = সত্য;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        যদি (keepp) {
            keepp->unlink_Fr_Back_With_পরবর্তী();
            nodep->replaceWith(keepp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

শ্রেণী Param_শীর্ষ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    শূন্য relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        জন্য (AstDot* ধ্রুবক dotp : m_state.m_dots) {
            ধ্রুবক AstClassOrPackageRef* ধ্রুবক classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ধ্রুবক AstClass* ধ্রুবক lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ধ্রুবক rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            জন্য (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                যদি (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    বিরতি;
                }
            }
        }
    }

    শূন্য fixLevel(Ast_গ্রন্থি_Module* modp) {
        // Fix up level of module, based on who instantiates it
        যদি (modp->user1SetOnce()) ফেরত;  // Already fixed
        যদি (m_state.m_parentps[modp].empty()) ফেরত;  // Leave top levels alone
        পূর্ণসংখ্যা maxParentLevel = 0;
        জন্য (Ast_গ্রন্থি_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        যদি (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    শূন্য resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_গ্রন্থি_Module*> modps;
        জন্য (Ast_গ্রন্থি_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), গ্রন্থি_Module);
            modp->unlinkFrBack();
            modps.ঠেলো_back(modp);
        }

        // Fix-up levels
        ধ্রুবক VNUser1InUse user1InUse;
        জন্য (Ast_গ্রন্থি_Module* ধ্রুবক modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ধ্রুবক Ast_গ্রন্থি_Module* ap, ধ্রুবক Ast_গ্রন্থি_Module* bp) {
                             ফেরত ap->level() < bp->level();
                         });

        // Re-insert modules
        জন্য (Ast_গ্রন্থি_Module* ধ্রুবক modp : modps) netlistp->addModulesp(modp);
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit Param_শীর্ষ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ধ্রুবক ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ধ্রুবক ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_গ্রন্থি_FTask_Ref* ftaskrefp) {
            Ast_গ্রন্থি_FTask* ftaskp = ftaskrefp->taskp();
            যদি (!ftaskp || !ftaskp->classMethod()) ফেরত;
            ধ্রুবক std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ধ্রুবক auto ancestorClassOf = [](Ast_গ্রন্থি* startp) -> AstClass* {
                জন্য (Ast_গ্রন্থি* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    যদি (AstClass* ধ্রুবক cp = VN_CAST(np, Class)) ফেরত cp;
                }
                ফেরত nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            যদি (refClassp == ftaskrefp->classOrPackagep())
                ফেরত;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            যদি (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                ফেরত;
            }
            যদি (classp->user3p()) ফেরত;  // will not get removed, no need to relink
            AstClass* ধ্রুবক parametrizedClassp = VN_CAST(classp->user4p(), Class);
            যদি (!parametrizedClassp) ফেরত;
            Ast_গ্রন্থি_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_গ্রন্থি_FTask* ftaskp) {
                যদি (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    ফেরত সত্য;
                }
                ফেরত মিথ্যা;
            });
            যদি (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ধ্রুবক ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        জন্য (AstClass* ধ্রুবক classp : m_state.m_paramClasses) {
            যদি (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                যদি (V3Error::error_গুনতি()) চালু;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ঠেলো_Deletep(classp->unlinkFrBack()), classp);
            } অন্যথা {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(মিথ্যা);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_গ্রন্থি::user3_Clear_বৃক্ষ();
        জন্য (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_গ্রন্থি* ধ্রুবক nodep) { nodep->user3(সত্য); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_গ্রন্থি* ধ্রুবক nodep) {
            nodep->foreachLink([&](Ast_গ্রন্থি** ধ্রুবক linkpp, ধ্রুবক অক্ষর* namep) {
                যদি (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_শীর্ষ() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(Param_শীর্ষ);
};

//######################################################################
// Param class functions

শূন্য V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    যদি (dump_বৃক্ষ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_শীর্ষ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    যদি (dump_বৃক্ষ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_বৃক্ষ("param", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    জন্য (AstVar* ধ্রুবক varp : rootp->deferredParamVarps()) {
        যদি (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
