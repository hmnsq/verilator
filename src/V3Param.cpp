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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Param.h"

#అన్తర్భావ "V3Case.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3HierBlock.h"
#అన్తర్భావ "V3LinkDotIfaceCapture.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3Simulate.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3Unroll.h"
#అన్తర్భావ "V3Width.h"

#అన్తర్భావ <cctype>
#అన్తర్భావ <cstdlib>
#అన్తర్భావ <deque>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <set>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
స్థిర AstClassRefDType* class_Ref_DType_యొక్క_కణుపు(Ast_కణుపు* nodep) {
    అయితే (AstTypedef* స్థిరరాశి tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(nodep, కణుపు_DType);
    తిరిగి dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : శూన్యసూచిక;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

తరగతి Parameterized_Hier_ఖండాలు final {
    ఉపయోగించి Hier_ఖండం_Opts_By_Orig_Name = std::multimap<std::string, స్థిరరాశి V3_క్రమానుగత_ఖండం_Option*>;
    ఉపయోగించి HierMapIt = Hier_ఖండం_Opts_By_Orig_Name::const_iterator;
    ఉపయోగించి Hier_ఖండం_Mod_Map = std::map<స్థిరరాశి std::string, Ast_కణుపు_Module*>;
    ఉపయోగించి ParamConstMap = std::map<స్థిరరాశి std::string, std::unique_ptr<AstConst>>;
    ఉపయోగించి GParamsMap = std::map<స్థిరరాశి std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    స్థిరరాశి తార్కిక m_hier_Sub_నిరంతరం;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_ఖండం_Opts_By_Orig_Name m_hier_ఖండం_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_ఖండం_Mod_Map m_hier_ఖండం_Mod;
    // Overridden parameters of the hierarchical block
    std::map<స్థిరరాశి V3_క్రమానుగత_ఖండం_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<స్థిరరాశి std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::సమితి<స్థిరరాశి AstVar*> m_untypedParams;

    // METHODS

బహిరంగ:
    Parameterized_Hier_ఖండాలు(స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_నిరంతరం{!v3Global.opt.hier_ఖండాలు().ఖాళీలు() || v3Global.opt.hier_బిడ్డ()} {
        కొరకు (స్థిరరాశి స్వయంచాలక& hierOpt : hierOpts) {
            m_hier_ఖండం_Opts_By_Orig_Name.emplace(hierOpt.రెండవ.origName(), &hierOpt.రెండవ);
            స్థిరరాశి V3_క్రమానుగత_ఖండం_Option::ParamStrMap& params = hierOpt.రెండవ.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.రెండవ];
            కొరకు (V3_క్రమానుగత_ఖండం_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.చివర(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    కొత్త File_పంక్తి{File_పంక్తి::built_లోపల_Filename()}, pIt->రెండవ)};
                UASSERT(constp, pIt->రెండవ << " is not a valid parameter literal");
                స్థిరరాశి తార్కిక inserted = consts.emplace(pIt->మొదటి, std::move(constp)).రెండవ;
                UASSERT(inserted, pIt->మొదటి << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.చొప్పించు({hierOpt.రెండవ.origName(), {}});
        }
        కొరకు (Ast_కణుపు_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), కణుపు_Module)) {
            ఒకవేళ (hierOpts.కనుగొను(modp->prettyName()) != hierOpts.చివర()) {
                m_hier_ఖండం_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            స్థిరరాశి string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            స్థిరరాశి స్వయంచాలక defParamIt = m_modParams.కనుగొను(actualModName);
            ఒకవేళ (defParamIt != m_modParams.చివర()) {
                // modp is the original of parameterized hierarchical block
                కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                        ఒకవేళ (varp->isGParam()) defParamIt->రెండవ.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        స్థిరరాశి AstBasicDType* స్థిరరాశి dtypep
                            = VN_CAST(varp->బిడ్డ_DTypep(), BasicDType);
                        ఒకవేళ (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    తార్కిక hier_Sub_నిరంతరం() స్థిరరాశి { తిరిగి m_hier_Sub_నిరంతరం; }
    తార్కిక is_Hier_ఖండం(స్థిరరాశి string& origName) స్థిరరాశి {
        తిరిగి m_hier_ఖండం_Opts_By_Orig_Name.కనుగొను(origName) != m_hier_ఖండం_Opts_By_Orig_Name.చివర();
    }
    Ast_కణుపు_Module* కనుగొను_By_Params(స్థిరరాశి string& origName, AstPin* మొదటి_Pinp,
                                స్థిరరాశి Ast_కణుపు_Module* modp) {
        UASSERT(is_Hier_ఖండం(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        స్థిరరాశి std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_ఖండం_Opts_By_Orig_Name.equal_range(origName);
        స్థిరరాశి స్వయంచాలక paramsIt = m_modParams.కనుగొను(origName);
        UASSERT_OBJ(paramsIt != m_modParams.చివర(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        స్థిరరాశి AstPin* typeMismatchPinp = శూన్యసూచిక;
        HierMapIt hierIt;
        కొరకు (hierIt = candidates.మొదటి; hierIt != candidates.రెండవ; ++hierIt) {
            తార్కిక దొరికింది = నిజం;
            కొలత_t paramIdx = 0;
            స్థిరరాశి ParamConstMap& params = m_hierParams[hierIt->రెండవ];
            UASSERT(params.కొలత() == hierIt->రెండవ->params().కొలత(), "not match");
            కొరకు (AstPin* pinp = మొదటి_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (!pinp->exprp()) కొనసాగు;
                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి modvarp = pinp->modVarp()) {
                    AstConst* స్థిరరాశి constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    స్థిరరాశి స్వయంచాలక paramIt = paramsIt->రెండవ.కనుగొను(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->రెండవ.చివర(), modvarp, "must be registered");
                    స్థిరరాశి తార్కిక untyped = m_untypedParams.లెక్క(paramIt->రెండవ);
                    AstConst* స్థిరరాశి defValuep = VN_CAST(paramIt->రెండవ->valuep(), Const);
                    ఒకవేళ (defValuep && areSame(constp, defValuep)) {
                        ఒకవేళ (!untyped || constp->same_విలువ_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            కొనసాగు;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    స్థిరరాశి స్వయంచాలక pIt = vlstd::as_const(params).కనుగొను(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    ఒకవేళ (pIt == params.చివర() || paramIdx >= params.కొలత()
                        || !areSame(constp, pIt->రెండవ.get())) {
                        దొరికింది = అబద్ధం;
                        ఆపు;
                    }
                    ఒకవేళ (untyped && !constp->same_విలువ_Type(pIt->రెండవ.get())) {
                        typeMismatchPinp = pinp;
                        దొరికింది = అబద్ధం;
                        ఆపు;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->రెండవ.get());
                    ++paramIdx;
                }
            }
            ఒకవేళ (దొరికింది && paramIdx == hierIt->రెండవ->params().కొలత()) ఆపు;
        }
        ఒకవేళ (hierIt == candidates.రెండవ && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_కణుపు::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            తిరిగి శూన్యసూచిక;
        }
        UASSERT_OBJ(hierIt != candidates.రెండవ, మొదటి_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        స్థిరరాశి Hier_ఖండం_Mod_Map::const_iterator modIt
            = m_hier_ఖండం_Mod.కనుగొను(hierIt->రెండవ->mangledName());
        UASSERT_OBJ(modIt != m_hier_ఖండం_Mod.చివర(), మొదటి_Pinp,
                    hierIt->రెండవ->mangledName() << " is not found");

        స్థిరరాశి స్వయంచాలక it = vlstd::as_const(m_hier_ఖండం_Mod).కనుగొను(hierIt->రెండవ->mangledName());
        ఒకవేళ (it == m_hier_ఖండం_Mod.చివర()) తిరిగి శూన్యసూచిక;
        తిరిగి it->రెండవ;
    }
    స్థిర తార్కిక areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        ఒకవేళ (pinValuep->isString()) {
            తిరిగి pinValuep->అంకె().గమ్యం_String() == hierOptParamp->అంకె().గమ్యం_String();
        }

        ఒకవేళ (hierOptParamp->isDouble()) {
            ద్విగుణ var;
            ఒకవేళ (pinValuep->isDouble()) {
                var = pinValuep->అంకె().గమ్యం_Double();
            } లేకపోతే {  // Cast from integer to real
                V3_సంఖ్య var_అంకె{pinValuep, V3_సంఖ్య::Double{}, 0.0};
                var_అంకె.opIToRD(pinValuep->అంకె());
                var = var_అంకె.గమ్యం_Double();
            }
            తిరిగి V3_సంఖ్య::epsilonEqual(var, hierOptParamp->అంకె().గమ్యం_Double());
        } లేకపోతే {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_సంఖ్య var_అంకె{pinValuep, hierOptParamp->అంకె().వెడల్పు()};
            ఒకవేళ (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_అంకె.opRToIRoundS(pinValuep->అంకె());
            } లేకపోతే ఒకవేళ (pinValuep->isSigned()) {
                var_అంకె.opExtendS(pinValuep->అంకె(), pinValuep->అంకె().వెడల్పు());
            } లేకపోతే {
                var_అంకె.op_కేటాయించు(pinValuep->అంకె());
            }
            V3_సంఖ్య isEq{pinValuep, 1};
            isEq.opEq(var_అంకె, hierOptParamp->అంకె());
            తిరిగి isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

తరగతి ParamProcessor final {
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
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    ఉపయోగించి IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    ఉపయోగించి CloneMap = std::unordered_map<స్థిరరాశి Ast_కణుపు*, Ast_కణుపు*>;
    నిర్మాణం ModInfo final {
        Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        స్పష్టమైన ModInfo(Ast_కణుపు_Module* modp)
            : m_modp{modp} {}
    };
    std::map<స్థిరరాశి std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<స్థిరరాశి std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::సమితి<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_సమితి m_all_Module_పేర్లు;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<స్థిరరాశి std::string, std::string> m_విలువ_పేర్లు;  // Parameter value text to its name
    std::సమితి<std::string> m_విలువ_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    స్థిర constexpr std::string::కొలత_type DIGEST_DIGITS = 8;

    స్థిరరాశి Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_ఖండాలు m_hier_ఖండాలు;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    ఉపయోగించి Default_విలువ_Map = std::map<std::string, Ast_కణుపు*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_కణుపు_Module*, Default_విలువ_Map> m_default_Parameter_విలువలు;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<స్థిరరాశి Ast_కణుపు_Module*, స్థిరరాశి AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::సదిశ<std::pair<AstParamTypeDType*, పూర్ణాంకం>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, పూర్ణాంకం> m_param_సూచీ;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_సమితి<స్థిరరాశి AstClass*> m_default_Clone_లోపల_పురోగతి;

    std::సదిశ<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    స్థిర శూన్య make_Small_పేర్లు(Ast_కణుపు_Module* modp) {
        std::సదిశ<పూర్ణాంకం> వాడిన_Letter;
        వాడిన_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isGParam() || varp->isIfaceRef()) {
                    అక్షరం ch = varp->name()[0];
                    ch = std::పెద్ద_అక్షరం(ch);
                    ఒకవేళ (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(వాడిన_Letter[static_cast<పూర్ణాంకం>(ch)] * 256 + ch);
                    వాడిన_Letter[static_cast<పూర్ణాంకం>(ch)]++;
                }
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి typep = VN_CAST(stmtp, ParamTypeDType)) {
                స్థిరరాశి అక్షరం ch = 'T';
                typep->user3(వాడిన_Letter[static_cast<పూర్ణాంకం>(ch)] * 256 + ch);
                వాడిన_Letter[static_cast<పూర్ణాంకం>(ch)]++;
            }
        }
    }
    స్థిర string paramSmallName(Ast_కణుపు_Module* modp, Ast_కణుపు* varp) {
        ఒకవేళ (varp->user3() <= 1) make_Small_పేర్లు(modp);
        పూర్ణాంకం సూచీ = varp->user3() / 256;
        స్థిరరాశి అక్షరం ch = varp->user3() & 255;
        string st = cvt_గమ్యం_Str(ch);
        అయితే (సూచీ) {
            st += cvt_గమ్యం_Str(static_cast<అక్షరం>((సూచీ % 25) + 'A'));
            సూచీ /= 26;
        }
        తిరిగి st;
    }

    స్థిర తార్కిక is_Aggregate_Param_విలువ(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    స్థిర string param_విలువ_String(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (స్థిరరాశి AstRefDType* స్థిరరాశి refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_గమ్యం_Non_Refp();
        }
        string కీలకం = nodep->name();
        ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            ఒకవేళ (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                కీలకం = ifrtp->cellp()->modp()->name();
            } లేకపోతే ఒకవేళ (ifrtp->ifacep()) {
                కీలకం = ifrtp->ifacep()->name();
            } లేకపోతే {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtypep
                   = VN_CAST(nodep, కణుపు_UOr_Struct_DType)) {
            కీలకం += " ";
            కీలకం += dtypep->verilogKwd();
            కీలకం += " {";
            కొరకు (స్థిరరాశి Ast_కణుపు* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                కీలకం += param_విలువ_String(memberp);
                కీలకం += ";";
            }
            కీలకం += "}";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberDType* స్థిరరాశి dtypep = VN_CAST(nodep, MemberDType)) {
            కీలకం += " ";
            కీలకం += param_విలువ_String(dtypep->subDTypep());
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి dtypep = VN_CAST(nodep, BasicDType)) {
            ఒకవేళ (dtypep->isSigned()) కీలకం += " signed";
            ఒకవేళ (dtypep->isRanged()) {
                కీలకం += "[" + cvt_గమ్యం_Str(dtypep->ఎడమ()) + ":" + cvt_గమ్యం_Str(dtypep->కుడి()) + "]";
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి dtypep = VN_CAST(nodep, PackArrayDType)) {
            కీలకం += "[";
            కీలకం += cvt_గమ్యం_Str(dtypep->ఎడమ());
            కీలకం += ":";
            కీలకం += cvt_గమ్యం_Str(dtypep->కుడి());
            కీలకం += "] ";
            కీలకం += param_విలువ_String(dtypep->subDTypep());
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_నమూనా* స్థిరరాశి patp = VN_CAST(nodep, నమూనా)) {
            // A parameter override pattern, keyed before the specialized module types it
            కీలకం += "'{";
            కొరకు (స్థిరరాశి AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                ఒకవేళ (memp->isDefault()) కీలకం += "default";
                ఒకవేళ (స్థిరరాశి Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(memp->keyp(), పాఠ్యం_2)) {
                    కీలకం += textp->పాఠ్యం();
                } లేకపోతే ఒకవేళ (memp->keyp()) {
                    కీలకం += param_విలువ_String(memp->keyp());
                }
                కీలకం += ":";
                ఒకవేళ (memp->repp()) కీలకం += param_విలువ_String(memp->repp()) + "x";
                కొరకు (స్థిరరాశి Ast_కణుపు_సమాసం* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), కణుపు_సమాసం)) {
                    కీలకం += param_విలువ_String(valuep) + ";";
                }
                కీలకం += ",";
            }
            కీలకం += "}";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            కీలకం += "{";
            కొరకు (స్థిరరాశి Ast_Cons_Array_అంశం* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_అంశం)) {
                కీలకం += cvt_గమ్యం_Str(itemp->సూచీ()) + ":" + param_విలువ_String(itemp->valuep()) + ",";
            }
            ఒకవేళ (consArrayp->defaultp()) {
                కీలకం += "default:" + param_విలువ_String(consArrayp->defaultp()) + ",";
            }
            కీలకం += "}";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstConsStruct* స్థిరరాశి consp = VN_CAST(nodep, ConsStruct)) {
            కీలకం += "{";
            కొరకు (స్థిరరాశి Ast_Cons_Struct_అంశం* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_అంశం)) {
                కీలకం += param_విలువ_String(itemp->valuep());
                కీలకం += ",";
            }
            కీలకం += "}";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            ఒకవేళ (classRefp->classp()) {
                స్థిరరాశి string& className = classRefp->classp()->name();
                స్థిరరాశి string& origName = classRefp->classp()->origName();
                స్థిరరాశి తార్కిక isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                ఒకవేళ (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } లేకపోతే ఒకవేళ (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    కీలకం = className;
                } లేకపోతే {
                    // Unspecialized class with no params - use origName
                    కీలకం = origName;
                }
            } లేకపోతే {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            కీలకం = paramElemString(dtypep->subDTypep()) + "$" + cvt_గమ్యం_Str(dtypep->declRange());
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి dtypep = VN_CAST(nodep, వరుస_DType)) {
            కీలకం = paramElemString(dtypep->subDTypep()) + "$[$";
            ఒకవేళ (dtypep->boundConst()) కీలకం += ":" + cvt_గమ్యం_Str(dtypep->boundConst());
            కీలకం += "]";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి dtypep = VN_CAST(nodep, DynArrayDType)) {
            కీలకం = paramElemString(dtypep->subDTypep()) + "$[]";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstAssocArrayDType* స్థిరరాశి dtypep = VN_CAST(nodep, AssocArrayDType)) {
            కీలకం = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->కీలకం_DTypep()) + "]";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstWildcardArrayDType* స్థిరరాశి dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            కీలకం = paramElemString(dtypep->subDTypep()) + "$[*]";
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstEnumDType* స్థిరరాశి dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            కీలకం += " enum {";
            కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(itemp->valuep(), Const);
                కీలకం += itemp->name() + "=" + (constp ? constp->అంకె().ascii() : "?") + ";";
            }
            కీలకం += "}";
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = VN_CAST(nodep, కణుపు_DType)) {
            కీలకం += dtypep->prettyDTypeName(నిజం);
        }
        UASSERT_OBJ(!కీలకం.ఖాళీలు(), nodep, "Parameter yielded no value string");
        తిరిగి కీలకం;
    }
    // As paramValueString(), for the element or key type of a container
    స్థిర string paramElemString(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp
            = VN_CAST(dtypep->skip_Ref_గమ్యం_Non_Refp(), ClassRefDType);
        ఒకవేళ (classRefp && classRefp->paramsp()) తిరిగి classRefp->prettyDTypeName(నిజం);
        తిరిగి param_విలువ_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    స్థిర string digestSuffix(స్థిరరాశి string& పాఠ్యం, std::సమితి<string>& usedr) {
        స్థిరరాశి string hex = VHashSha512{పాఠ్యం}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::కొలత_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        అయితే (digits < hex.కొలత() && !usedr.చొప్పించు(hex.substr(0, digits)).రెండవ) ++digits;
        తిరిగి hex.substr(0, digits);
    }
    string param_విలువ_సంఖ్య(Ast_కణుపు* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        ఒకవేళ (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        స్థిరరాశి string పాఠ్యం = param_విలువ_String(nodep);
        స్థిరరాశి స్వయంచాలక pair = m_విలువ_పేర్లు.emplace(పాఠ్యం, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ = "z" + digestSuffix(పాఠ్యం, m_విలువ_Suffixes);
        తిరిగి pair.మొదటి->రెండవ;
    }
    string moduleCalcName(స్థిరరాశి Ast_కణుపు_Module* srcModp, స్థిరరాశి string& longname) {
        string newname = longname;
        ఒకవేళ (longname.నిడివి() > 30) {
            స్థిరరాశి స్వయంచాలక pair = m_longMap.emplace(longname, "");
            ఒకవేళ (pair.రెండవ) {
                // We use all upper case above, so lower here can't conflict
                pair.మొదటి->రెండవ
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.మొదటి->రెండవ;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        తిరిగి newname;
    }
    స్థిర Ast_కణుపు_DType* arraySubDTypep(Ast_కణుపు_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            తిరిగి adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        ఒకవేళ (స్థిరరాశి AstBracketArrayDType* స్థిరరాశి adtypep = VN_CAST(nodep, BracketArrayDType)) {
            తిరిగి adtypep->subDTypep();
        }
        తిరిగి శూన్యసూచిక;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    స్థిర Ast_కణుపు_DType* arraySubDTypeDeepp(Ast_కణుపు_DType* nodep) {
        అయితే (Ast_కణుపు_DType* స్థిరరాశి subp = arraySubDTypep(nodep)) nodep = subp;
        తిరిగి nodep;
    }
    స్థిర తార్కిక isString(Ast_కణుపు_DType* nodep) {
        ఒకవేళ (AstBasicDType* స్థిరరాశి basicp = VN_CAST(nodep->skip_Ref_గమ్యం_Non_Refp(), BasicDType))
            తిరిగి basicp->isString();
        తిరిగి అబద్ధం;
    }
    శూన్య collectPins(CloneMap* clonemapp, Ast_కణుపు_Module* modp, తార్కిక originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            స్థిరరాశి Ast_కణుపు* originalParamp = శూన్యసూచిక;
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ఒకవేళ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ఒకవేళ (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            ఒకవేళ (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    శూన్య relinkPins(స్థిరరాశి CloneMap* clonemapp, AstPin* startpinp) {
        కొరకు (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                స్థిరరాశి స్వయంచాలక cloneiter = clonemapp->కనుగొను(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->చివర(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->రెండవ, Var));
            } లేకపోతే ఒకవేళ (pinp->modPTypep()) {
                స్థిరరాశి స్వయంచాలక cloneiter = clonemapp->కనుగొను(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->చివర(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->రెండవ, ParamTypeDType));
            } లేకపోతే {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    శూన్య relinkPinsByName(AstPin* startpinp, Ast_కణుపు_Module* modp) {
        std::map<స్థిరరాశి string, AstVar*> name_గమ్యం_Pin;
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_గమ్యం_Pin.emplace(varp->name(), varp);
                }
            }
        }
        కొరకు (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = pinp->modVarp()) {
                స్థిరరాశి స్వయంచాలక varIt = vlstd::as_const(name_గమ్యం_Pin).కనుగొను(varp->name());
                UASSERT_OBJ(varIt != name_గమ్యం_Pin.చివర(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->రెండవ);
            }
        }
    }
    స్థిర తార్కిక hasDescendantDefparams(స్థిరరాశి Ast_కణుపు_Module* modp,
                                       std::సమితి<స్థిరరాశి Ast_కణుపు_Module*>& సందర్శించిన) {
        ఒకవేళ (!సందర్శించిన.చొప్పించు(modp).రెండవ) తిరిగి అబద్ధం;
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* స్థిరరాశి cellp = VN_CAST(stmtp, Cell);
            ఒకవేళ (!cellp) కొనసాగు;
            కొరకు (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (!pinp->param_మార్గం().ఖాళీలు()) {
                    సందర్శించిన.erase(modp);
                    తిరిగి నిజం;
                }
            }
            ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి బిడ్డ_Modp = cellp->modp()) {
                ఒకవేళ (hasDescendantDefparams(బిడ్డ_Modp, సందర్శించిన)) {
                    సందర్శించిన.erase(modp);
                    తిరిగి నిజం;
                }
            }
        }
        సందర్శించిన.erase(modp);
        తిరిగి అబద్ధం;
    }
    స్థిర తార్కిక hasDescendantDefparams(స్థిరరాశి Ast_కణుపు_Module* modp) {
        std::సమితి<స్థిరరాశి Ast_కణుపు_Module*> సందర్శించిన;
        తిరిగి hasDescendantDefparams(modp, సందర్శించిన);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    స్థిరరాశి AstPin* nestedDefparamp(స్థిరరాశి Ast_కణుపు_Module* modp) {
        స్థిరరాశి స్వయంచాలక pair = m_nestedDefparamps.emplace(modp, శూన్యసూచిక);
        ఒకవేళ (!pair.రెండవ) తిరిగి pair.మొదటి->రెండవ;  // Searched, or being searched
        స్థిరరాశి AstPin* foundp = శూన్యసూచిక;
        modp->exists([&](స్థిరరాశి AstCell* cellp) {
            కొరకు (స్థిరరాశి AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (!pinp->param_మార్గం().ఖాళీలు()) foundp = pinp;
            }
            ఒకవేళ (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            తిరిగి foundp != శూన్యసూచిక;
        });
        pair.మొదటి->రెండవ = foundp;
        తిరిగి foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    శూన్య checkSupportedParam(Ast_కణుపు_Module* modp, AstPin* pinp) స్థిరరాశి {
        // ConsArray is not supported because that can not be set via -G
        // option.
        ఒకవేళ (pinp->modVarp()) {
            తార్కిక supported = అబద్ధం;
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(pinp->exprp(), Const)) {
                supported = నిజం;
                ఒకవేళ (constp->అంకె().isString()
                    && !V3_Hier_ఖండం::stringParamPassable(constp->అంకె().గమ్యం_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            ఒకవేళ (!supported) {
                pinp->v3error(
                    Ast_కణుపు::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstParamTypeDType* స్థిరరాశి typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_కణుపు::prettyNameQ(modp->origName()));
        }
    }
    తార్కిక moduleExists(స్థిరరాశి string& modName) స్థిరరాశి {
        ఒకవేళ (m_all_Module_పేర్లు.కనుగొను(modName) != m_all_Module_పేర్లు.చివర()) తిరిగి నిజం;
        ఒకవేళ (m_modNameMap.కనుగొను(modName) != m_modNameMap.చివర()) తిరిగి నిజం;
        తిరిగి అబద్ధం;
    }

    string parameterized_Hier_ఖండం_Name(Ast_కణుపు_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_ఖండం(), modp, "should be used for hierarchical block");

        std::map<string, Ast_కణుపు*> pins;

        AstPin* pinp = paramPinsp;
        అయితే (pinp) {
            checkSupportedParam(modp, pinp);
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = pinp->modVarp()) {
                ఒకవేళ (!pinp->exprp()) కొనసాగు;
                ఒకవేళ (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } లేకపోతే ఒకవేళ (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), కణుపు_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        స్థిరరాశి స్వయంచాలక pair = m_default_Parameter_విలువలు.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        ఒకవేళ (pair.రెండవ) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            ఒకవేళ (స్థిరరాశి AstPin* స్థిరరాశి defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_కణుపు::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_విలువ_Map params;
            కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                    ఒకవేళ (varp->isGParam()) {
                        AstConst* స్థిరరాశి constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_కణుపు* స్థిరరాశి dtypep = static_cast<Ast_కణుపు*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.మొదటి->రెండవ = std::move(params);
        }
        స్థిరరాశి స్వయంచాలక paramsIt = pair.మొదటి;
        ఒకవేళ (paramsIt->రెండవ.ఖాళీలు()) తిరిగి modp->origName();  // modp has no parameter

        string longname = modp->origName();
        కొరకు (స్వయంచాలక&& default_విలువ : paramsIt->రెండవ) {
            స్థిరరాశి స్వయంచాలక pinIt = pins.కనుగొను(default_విలువ.మొదటి);
            // If the pin does not have a value assigned, use the default one.
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep = pinIt == pins.చివర() ? default_విలువ.రెండవ : pinIt->రెండవ;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_విలువ.మొదటి + "=";
            // constp can be nullptr

            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->అంకె().ascii(అబద్ధం);
            } లేకపోతే ఒకవేళ (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_వృక్షం(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        స్థిరరాశి స్వయంచాలక iter = m_longMap.కనుగొను(longname);
        ఒకవేళ (iter != m_longMap.చివర()) తిరిగి iter->రెండవ;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.చొప్పించు(longname);
        అయితే (నిజం) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            స్థిరరాశి string hashStr = hashStrGen.digestHex();
            కొరకు (string::కొలత_type i = 1; i < hashStr.కొలత(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                ఒకవేళ (newName.at(newName.కొలత() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                ఒకవేళ (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    తిరిగి newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.చొప్పించు(V3Os::trueRandom(64));
        }
    }
    శూన్య replaceRefsRecurse(Ast_కణుపు* స్థిరరాశి nodep, స్థిరరాశి AstClass* స్థిరరాశి oldClassp,
                            AstClass* స్థిరరాశి newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(nodep, ClassRefDType)) {
            ఒకవేళ (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } లేకపోతే ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ఒకవేళ (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } లేకపోతే ఒకవేళ (AstTypedef* స్థిరరాశి typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            ఒకవేళ (typedefp->subDTypep()) {
                ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    ఒకవేళ (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskRefp = VN_CAST(nodep, కణుపు_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            ఒకవేళ (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి oldTaskp = ftaskRefp->taskp()) {
                ఒకవేళ (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        ఒకవేళ (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        ఒకవేళ (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        ఒకవేళ (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        ఒకవేళ (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        ఒకవేళ (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    తరగతి VarXRefRelinkVisitor final : బహిరంగ VNVisitor {
        Ast_కణుపు_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_కణుపు_Module*> m_varModuleMap;  // Cache var->module lookups

    బహిరంగ:
        స్పష్టమైన VarXRefRelinkVisitor(Ast_కణుపు_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    అంతరంగ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_కణుపు_Module* కనుగొను_Var_Module(AstVar* varp) {
            స్థిరరాశి స్వయంచాలక it = m_varModuleMap.కనుగొను(varp);
            ఒకవేళ (it != m_varModuleMap.చివర()) తిరిగి it->రెండవ;
            Ast_కణుపు_Module* varModp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు* np = varp; np; np = np->backp()) {
                ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి modp = VN_CAST(np, కణుపు_Module)) {
                    varModp = modp;
                    ఆపు;
                }
            }
            m_varModuleMap[varp] = varModp;
            తిరిగి varModp;
        }

        శూన్య సందర్శించు(AstVarXRef* nodep) override {
            AstVar* స్థిరరాశి varp = nodep->varp();
            ఒకవేళ (!varp) {
                iterateChildren(nodep);
                తిరిగి;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            స్థిరరాశి string& dotted = nodep->dotted();
            ఒకవేళ (dotted.ఖాళీలు()) {
                iterateChildren(nodep);
                తిరిగి;
            }

            స్థిరరాశి కొలత_t dotPos = dotted.కనుగొను('.');
            స్థిరరాశి string పోర్టు_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            ఒకవేళ (పోర్టు_Name.ఖాళీలు()) {
                iterateChildren(nodep);
                తిరిగి;
            }

            // Find the interface port variable in the cloned module
            AstVar* పోర్టు_Varp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (AstVar* స్థిరరాశి varChkp = VN_CAST(stmtp, Var)) {
                    ఒకవేళ (varChkp->name() == పోర్టు_Name && varChkp->isIfaceRef()) {
                        పోర్టు_Varp = varChkp;
                        ఆపు;
                    }
                }
            }
            ఒకవేళ (!పోర్టు_Varp) {
                iterateChildren(nodep);
                తిరిగి;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* స్థిరరాశి irefp = VN_CAST(పోర్టు_Varp->subDTypep(), IfaceRefDType);
            ఒకవేళ (!irefp) {
                iterateChildren(nodep);
                తిరిగి;
            }

            Ast_కణుపు_Module* స్థిరరాశి newIfacep = irefp->iface_ద్వారా_Cellp();
            ఒకవేళ (!newIfacep) {
                iterateChildren(nodep);
                తిరిగి;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_కణుపు_Module* స్థిరరాశి varModp = కనుగొను_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            ఒకవేళ (varModp && varModp != newIfacep) {
                కొరకు (Ast_కణుపు* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (AstVar* స్థిరరాశి newVarp = VN_CAST(stmtp, Var)) {
                        ఒకవేళ (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            ఆపు;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    శూన్య fixupCrossModuleRefDTypes(Ast_కణుపు_Module* newModp) {
        ఒకవేళ (!V3LinkDotIfaceCapture::enabled()) తిరిగి;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        పూర్ణాంకం fixed_లెక్క = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = V3LinkDotIfaceCapture::capture_ట్యాగ్(refp);
            ఒకవేళ (!tagp) తిరిగి;

            Ast_కణుపు_Module* స్థిరరాశి correctModp
                = V3LinkDotIfaceCapture::follow_Cell_మార్గం(newModp, tagp->m_cell_మార్గం);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_మార్గం << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            ఒకవేళ (!correctModp || correctModp->dead()) తిరిగి;
            ఒకవేళ (correctModp->parameterizedTemplate()) తిరిగి;

            తార్కిక fixed = అబద్ధం;
            ఒకవేళ (refp->typedefp()) {
                ఒకవేళ (AstTypedef* స్థిరరాశి newTdp = V3LinkDotIfaceCapture::కనుగొను_Typedef_లోపల_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    ఒకవేళ (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = నిజం;
                }
            }
            ఒకవేళ (!fixed && refp->refDTypep()) {
                ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి newDtp = V3LinkDotIfaceCapture::కనుగొను_DType_లోపల_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = నిజం;
                }
            }
            ఒకవేళ (fixed) ++fixed_లెక్క;
        });
        V3Stats::కూడు_Stat_కూడిక("IfaceCapture, Ledger fixups in V3Param", fixed_లెక్క);
    }

    // Return true on success, false on error
    తార్కిక deepCloneModule(Ast_కణుపు_Module* srcModp, Ast_కణుపు* ifErrorp, AstPin* paramsp,
                         స్థిరరాశి string& newname, స్థిరరాశి IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_కణుపు_Module* newModp;
        ఒకవేళ (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_వృక్షం(అబద్ధం), కణుపు_Module);
        } లేకపోతే {
            newModp = srcModp->clone_వృక్షం(అబద్ధం);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        ఒకవేళ (AstIface* స్థిరరాశి newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(నిజం);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(అబద్ధం);

        newModp->name(newname);
        newModp->user2(అబద్ధం);  // We need to re-recurse this module once changed
        newModp->recursive(అబద్ధం);
        newModp->recursiveClone(అబద్ధం);
        newModp->hasGParam(అబద్ధం);
        // Recursion may need level cleanups
        ఒకవేళ (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        ఒకవేళ ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_లోతు() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_లోతు());
            VL_DO_DANGLING(newModp->delete_వృక్షం(), newModp);
            తిరిగి అబద్ధం;
        }

        ఒకవేళ (AstClass* స్థిరరాశి newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_కణుపు* insertp = srcModp;
        అయితే (VN_IS(insertp->nextp(), కణుపు_Module)
               && VN_AS(insertp->nextp(), కణుపు_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->కూడు_తదుపరి_ఇక్కడ(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        స్థిరరాశి స్వయంచాలక iter = m_modNameMap.కనుగొను(newname);
        CloneMap* స్థిరరాశి clonemapp = &(iter->రెండవ.m_cloneMap);
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
        తార్కిక needRelinkByName = అబద్ధం;
        ఒకవేళ (paramsp) {
            కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (pinp->modVarp() && clonemapp->కనుగొను(pinp->modVarp()) == clonemapp->చివర()) {
                    needRelinkByName = నిజం;
                    ఆపు;
                }
            }
        }
        ఒకవేళ (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } లేకపోతే {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        కొరకు (స్వయంచాలక it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి పోర్టు_Irefp = it->మొదటి;
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి pinIrefp = it->రెండవ;
            AstIfaceRefDType* స్థిరరాశి cloneIrefp = పోర్టు_Irefp->clonep();
            UINFO(8, "     IfaceOld " << పోర్టు_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, పోర్టు_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_ద్వారా_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        ఒకవేళ (!ifaceRefRefs.ఖాళీలు()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (pinp->exprp()) {
                ఒకవేళ (AstVar* స్థిరరాశి modvarp = pinp->modVarp()) {
                    Ast_కణుపు* స్థిరరాశి newp = pinp->exprp();  // Const or ConsArray
                    AstConst* స్థిరరాశి exprp = VN_CAST(newp, Const);
                    AstConst* స్థిరరాశి origp = VN_CAST(modvarp->valuep(), Const);
                    స్థిరరాశి తార్కిక overridden
                        = !(origp && exprp && Parameterized_Hier_ఖండాలు::areSame(exprp, origp));
                    // Remove any existing parameter
                    ఒకవేళ (modvarp->valuep()) modvarp->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_వృక్షం(అబద్ధం));
                    modvarp->overriddenParam(overridden);
                } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి modptp = pinp->modPTypep()) {
                    Ast_కణుపు_DType* స్థిరరాశి dtypep = VN_AS(pinp->exprp(), కణుపు_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    ఒకవేళ (modptp->బిడ్డ_DTypep()) modptp->బిడ్డ_DTypep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                    // Set this parameter to value requested by cell
                    modptp->బిడ్డ_DTypep(dtypep->clone_వృక్షం(అబద్ధం));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        తిరిగి నిజం;
    }
    స్థిరరాశి ModInfo* module_కనుగొను_Or_Clone(Ast_కణుపు_Module* srcModp, Ast_కణుపు* ifErrorp, AstPin* paramsp,
                                     స్థిరరాశి string& newname, స్థిరరాశి IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        స్వయంచాలక it = m_modNameMap.కనుగొను(newname);
        ఒకవేళ (it != m_modNameMap.చివర()) {
            UINFO(4, "     De-parameterize to prev: " << it->రెండవ.m_modp);
        } లేకపోతే {
            ఒకవేళ (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                తిరిగి శూన్యసూచిక;
            }
            it = m_modNameMap.కనుగొను(newname);
            UASSERT(it != m_modNameMap.చివర(), "should find just-made module");
        }
        స్థిరరాశి ModInfo* స్థిరరాశి modInfop = &(it->రెండవ);
        తిరిగి modInfop;
    }

    శూన్య convert_గమ్యం_Stringp(Ast_కణుపు* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        ఒకవేళ (constp && !constp->అంకె().isString()) {
            constp->replaceWith(
                కొత్త AstConst{constp->fileline(), AstConst::String{}, constp->అంకె().గమ్యం_String()});
            VL_DO_DANGLING(constp->delete_వృక్షం(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    శూన్య constify_Member_విలువ(AstVar* varp, std::సమితి<AstVar*>& లోపల_పురోగతి) {
        ఒకవేళ (!varp->valuep() || VN_IS(varp->valuep(), Const)) తిరిగి;
        ఒకవేళ (!లోపల_పురోగతి.చొప్పించు(varp).రెండవ) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
            varp->valuep(కొత్త AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            తిరిగి;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::సమితి<AstVar*> siblings;
        varp->valuep()->foreach([&](స్థిరరాశి AstVarRef* refp) {
            AstVar* స్థిరరాశి refVarp = refp->varp();
            ఒకవేళ (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.చొప్పించు(refVarp);
            }
        });
        కొరకు (AstVar* స్థిరరాశి sibp : siblings) constify_Member_విలువ(sibp, లోపల_పురోగతి);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        లోపల_పురోగతి.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    స్థిర AstParseRef* memberParseRef(Ast_కణుపు* nodep) {
        ఒకవేళ (AstParseRef* స్థిరరాశి refp = VN_CAST(nodep, ParseRef)) తిరిగి refp;
        Ast_కణుపు_Pre_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Pre_Sel);
        తిరిగి selp ? VN_CAST(selp->fromp(), ParseRef) : శూన్యసూచిక;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    స్థిర శూన్య replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } లేకపోతే {
            Ast_కణుపు_Pre_Sel* స్థిరరాశి selp = VN_AS(dotp->rhsp(), కణుపు_Pre_Sel);
            ఒకవేళ (Ast_Attr_యొక్క* స్థిరరాశి attrp = selp->attrp()) {
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_వృక్షం(అబద్ధం));
                    VL_DO_DANGLING(oldFromp->delete_వృక్షం(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_వృక్షం(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_వెనుక());
        }
        VL_DO_DANGLING(dotp->delete_వృక్షం(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    స్థిర తార్కిక is_Deferred_Member_పునాది(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (VN_IS(nodep, MemberSel)) తిరిగి నిజం;
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Pre_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Pre_Sel)) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDTypep = selp->fromp()->dtypep();
            స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), కణుపు_Array_DType) : శూన్యసూచిక;
            ఒకవేళ (arrayDTypep) {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDTypep = arrayDTypep->subDTypep();
                తిరిగి elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), కణుపు_UOr_Struct_DType);
            }
            తిరిగి is_Deferred_Member_పునాది(selp->fromp());
        }
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->dtypep();
        తిరిగి dtypep && VN_IS(dtypep->skipRefOrNullp(), కణుపు_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    స్థిర శూన్య resolveMemberDot(AstDot* dotp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = VN_CAST(dotp->lhsp(), కణుపు_సమాసం);
        AstParseRef* స్థిరరాశి memberRefp = memberParseRef(dotp->rhsp());
        ఒకవేళ (!lhsp || !memberRefp || !is_Deferred_Member_పునాది(lhsp)) తిరిగి;
        AstMemberSel* స్థిరరాశి newp = కొత్త AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_వెనుక(),
                                                    VFlag_బిడ్డ_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    శూన్య resolve_Dot_గమ్యం_Typedef(Ast_కణుపు* exprp) {
        AstDot* స్థిరరాశి dotp = VN_CAST(exprp, Dot);
        ఒకవేళ (!dotp) తిరిగి;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* స్థిరరాశి classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        ఒకవేళ (!classRefp) {
            resolveMemberDot(dotp);
            తిరిగి;
        }
        AstParseRef* స్థిరరాశి parseRefp = memberParseRef(dotp->rhsp());
        ఒకవేళ (!parseRefp) తిరిగి;

        స్థిరరాశి AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* స్థిరరాశి paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        ఒకవేళ (paramTypep) {
            Ast_కణుపు_DType* స్థిరరాశి dtypep = paramTypep->subDTypep();
            AstClassRefDType* స్థిరరాశి classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : శూన్యసూచిక;
            ఒకవేళ (classRefDTypep) {
                AstClass* స్థిరరాశి srcClassp = classRefDTypep->classp();
                ఒకవేళ (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    ఒకవేళ (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        ఒకవేళ (classRefp->paramsp()) {
            AstClass* స్థిరరాశి srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            ఒకవేళ (srcClassp && srcClassp->hasGParam()) {
                ఒకవేళ (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        ఒకవేళ (AstClassRefDType* స్థిరరాశి crp = class_Ref_DType_యొక్క_కణుపు(classRefp->classOrPackageNodep())) {
            ఒకవేళ (AstClass* స్థిరరాశి srcClassp = crp->classp()) {
                ఒకవేళ (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    ఒకవేళ (AstClass* స్థిరరాశి newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        ఒకవేళ (!lhsClassp) తిరిగి;

        Ast_కణుపు* స్థిరరాశి memberp = m_memberMap.కనుగొను_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        ఒకవేళ (AstTypedef* స్థిరరాశి tdefp = VN_CAST(memberp, Typedef)) {
            ఒకవేళ (parseRefp != dotp->rhsp()) తిరిగి;
            AstRefDType* స్థిరరాశి refp = కొత్త AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_వృక్షం(), dotp);
        } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    శూన్య substituteParamMember(AstDot* స్థిరరాశి dotp, AstParseRef* స్థిరరాశి memberRefp,
                               AstVar* స్థిరరాశి varp) {
        ఒకవేళ (!varp->isParam() || !varp->valuep()) తిరిగి;
        ఒకవేళ (!VN_IS(varp->valuep(), Const)) {
            std::సమితి<AstVar*> లోపల_పురోగతి;
            constify_Member_విలువ(varp, లోపల_పురోగతి);
        }
        AstConst* స్థిరరాశి constp = VN_CAST(varp->valuep(), Const);
        ఒకవేళ (!constp) తిరిగి;
        AstConst* స్థిరరాశి newp = constp->clone_వృక్షం(అబద్ధం);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    శూన్య resolveParamClassRefDType(AstRefDType* refp) {
        ఒకవేళ (refp->typedefp() || refp->refDTypep()) తిరిగి;

        AstClassOrPackageRef* స్థిరరాశి classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        ఒకవేళ (!classRefp) తిరిగి;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        ఒకవేళ (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            తిరిగి;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_కణుపు* targetp = classRefp->classOrPackageNodep();
        అయితే (స్థిరరాశి AstTypedef* స్థిరరాశి tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtargetp = VN_CAST(targetp, కణుపు_DType))
            targetp = dtargetp->skipRefOrNullp();
        ఒకవేళ (!targetp) తిరిగి;
        AstClassRefDType* స్థిరరాశి classRefDTypep = VN_CAST(targetp, ClassRefDType);
        ఒకవేళ (!classRefDTypep) తిరిగి;
        srcClassp = classRefDTypep->classp();
        ఒకవేళ (!srcClassp) తిరిగి;

        AstClass* newClassp = srcClassp;
        ఒకవేళ (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* స్థిరరాశి typedefp
            = VN_CAST(m_memberMap.కనుగొను_Member(newClassp, refp->name()), Typedef);
        ఒకవేళ (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    స్థిర తార్కిక defaultParamsHaveTypeQueryOnParamType(స్థిరరాశి AstClassRefDType* nodep) {
        తార్కిక దొరికింది = అబద్ధం;
        nodep->foreach([&](స్థిరరాశి Ast_Attr_యొక్క* attrp) {
            ఒకవేళ (దొరికింది || !attrp->attrType().isTypeQuery()) తిరిగి;
            స్థిరరాశి AstRefDType* స్థిరరాశి refp = VN_CAST(attrp->fromp(), RefDType);
            ఒకవేళ (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) దొరికింది = నిజం;
        });
        తిరిగి దొరికింది;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    తార్కిక classTypeMatchesDefaultClone(స్థిరరాశి Ast_కణుపు_DType* exprp, స్థిరరాశి Ast_కణుపు_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        స్థిరరాశి స్వయంచాలక* స్థిరరాశి సమాసం_Class_Refp = VN_CAST(exprp, ClassRefDType);
        స్థిరరాశి స్వయంచాలక* స్థిరరాశి origClassRefp = VN_CAST(origp, ClassRefDType);
        ఒకవేళ (!సమాసం_Class_Refp || !origClassRefp) తిరిగి అబద్ధం;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        ఒకవేళ (defaultClonep && defaultClonep == సమాసం_Class_Refp->classp()) తిరిగి నిజం;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        ఒకవేళ (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) తిరిగి అబద్ధం;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        ఒకవేళ (!origClassRefp->classp()->hasGParam()) తిరిగి అబద్ధం;
        ఒకవేళ (origClassRefp->classp()->origName() != సమాసం_Class_Refp->classp()->origName())
            తిరిగి అబద్ధం;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        ఒకవేళ (!m_default_Clone_లోపల_పురోగతి.చొప్పించు(origClassRefp->classp()).రెండవ) తిరిగి అబద్ధం;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* స్థిరరాశి origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_వృక్షం(అబద్ధం));
        Ast_కణుపు_Module* స్థిరరాశి resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        స్థిరరాశి తార్కిక సరిపోలిక = resolvedModp && VN_CAST(resolvedModp, Class) == సమాసం_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_వృక్షం(), origClonep);
        m_default_Clone_లోపల_పురోగతి.erase(origClassRefp->classp());
        తిరిగి సరిపోలిక;
    }

    స్థిర తార్కిక param_Consts_Equal_At_Max_వెడల్పు(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        ఒకవేళ (exprp->అంకె().వెడల్పు() == origp->అంకె().వెడల్పు()) తిరిగి అబద్ధం;
        ఒకవేళ (exprp->అంకె().isOpaque()) తిరిగి అబద్ధం;
        ఒకవేళ (origp->అంకె().isOpaque()) తిరిగి అబద్ధం;
        స్థిరరాశి పూర్ణాంకం max_వెడల్పు = std::max(exprp->అంకె().వెడల్పు(), origp->అంకె().వెడల్పు());
        V3_సంఖ్య సమాసం_అంకె{exprp, max_వెడల్పు};
        ఒకవేళ (exprp->అంకె().isSigned()) {
            సమాసం_అంకె.opExtendS(exprp->అంకె(), exprp->అంకె().వెడల్పు());
        } లేకపోతే {
            సమాసం_అంకె.op_కేటాయించు(exprp->అంకె());
        }
        V3_సంఖ్య orig_అంకె{origp, max_వెడల్పు};
        ఒకవేళ (origp->అంకె().isSigned()) {
            orig_అంకె.opExtendS(origp->అంకె(), origp->అంకె().వెడల్పు());
        } లేకపోతే {
            orig_అంకె.op_కేటాయించు(origp->అంకె());
        }
        V3_సంఖ్య isEq{exprp, 1};
        isEq.opEq(సమాసం_అంకె, orig_అంకె);
        తిరిగి isEq.isNeqZero();
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
    శూన్య resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (!pinp->exprp()) కొనసాగు;
            ఒకవేళ (!VString::startsWith(pinp->name(), "__VGIfaceParam")) కొనసాగు;
            స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి fwdRefp = VN_CAST(pinp->exprp(), కణుపు_Var_Ref);
            ఒకవేళ (!fwdRefp) కొనసాగు;
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి resolvedp
                = VN_CAST(fwdRefp->varp()->బిడ్డ_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* స్థిరరాశి newIrefp = కొత్త AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            ఒకవేళ (resolvedp->paramsp()) {
                newIrefp->కూడు_Paramsp(resolvedp->paramsp()->clone_వృక్షం(నిజం));
            }
            pinp->exprp()->unlink_Fr_వెనుక()->delete_వృక్షం();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    శూన్య name_By_Pin_విలువ(AstPin* pinp, Ast_కణుపు_Module* srcModp, AstVar* modvarp, string& longnamer,
                        తార్కిక& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_విలువ_సంఖ్య(pinp->exprp());
        any_overridesr = నిజం;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    శూన్య cellPinCleanup(Ast_కణుపు* nodep, AstPin* pinp, AstPin* paramsp, Ast_కణుపు_Module* srcModp,
                        string& longnamer, తార్కిక& any_overridesr,
                        std::unordered_సమితి<స్థిరరాశి AstPin*>& overridingTypePinsr) {
        ఒకవేళ (!pinp->exprp()) తిరిగి;  // No-connect
        ఒకవేళ (AstVar* స్థిరరాశి modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            ఒకవేళ (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } లేకపోతే ఒకవేళ (VN_IS(pinp->exprp(), నమూనా)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_విలువ(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } లేకపోతే {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                ఒకవేళ (is_Aggregate_Param_విలువ(pinp->exprp())) {
                    name_By_Pin_విలువ(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    తిరిగి;
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
                ఒకవేళ (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_కొలత))) {
                    స్థిరరాశి తార్కిక dependent = modvarp->valuep()->exists(
                        [](Ast_కణుపు* np) { తిరిగి VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    ఒకవేళ (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                ఒకవేళ (isString(modvarp->subDTypep())) {
                    convert_గమ్యం_Stringp(pinp->exprp());
                    convert_గమ్యం_Stringp(modvarp->valuep());
                }
                AstConst* స్థిరరాశి exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* స్థిరరాశి origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = శూన్యసూచిక;
                ఒకవేళ (exprp && !exprp->అంకె().isDouble() && !exprp->అంకె().isString()) {
                    AstVar* cloneVarp = modvarp->clone_వృక్షం(అబద్ధం);
                    తార్కిక cloneVarpUnresolved = అబద్ధం;
                    ఒకవేళ (Ast_కణుపు* స్థిరరాశి oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_వెనుక();
                        VL_DO_DANGLING(oldValuep->delete_వృక్షం(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_వృక్షం(అబద్ధం));
                    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        ఒకవేళ (cloneVarp->బిడ్డ_DTypep())
                            cloneVarp->బిడ్డ_DTypep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                        cloneVarp->బిడ్డ_DTypep(origDTypep->clone_వృక్షం(అబద్ధం));
                        cloneVarp->dtypep(శూన్యసూచిక);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr పూర్ణాంకం maxSubstIters = 1000;
                        కొరకు (పూర్ణాంకం it = 0; it < maxSubstIters; ++it) {
                            తార్కిక any = అబద్ధం;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* స్థిరరాశి targetp = varrefp->varp();
                                Ast_కణుపు* replacep = శూన్యసూచిక;
                                కొరకు (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ఒకవేళ (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        ఒకవేళ (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), నమూనా)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_వృక్షం(అబద్ధం);
                                        }
                                        ఆపు;
                                    }
                                }
                                ఒకవేళ (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_వృక్షం(అబద్ధం);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_నమూనా* స్థిరరాశి patp = VN_CAST(replacep, నమూనా);
                                ఒకవేళ (patp && !patp->బిడ్డ_DTypep() && targetp->బిడ్డ_DTypep()) {
                                    patp->బిడ్డ_DTypep(targetp->బిడ్డ_DTypep()->clone_వృక్షం(అబద్ధం));
                                }
                                ఒకవేళ (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_వృక్షం(), varrefp);
                                    any = నిజం;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::సదిశ<std::pair<AstRefDType*, Ast_కణుపు_DType*>> గమ్యం_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* స్థిరరాశి ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                ఒకవేళ (!ptdp) తిరిగి;
                                AstPin* overridePinp = శూన్యసూచిక;
                                కొరకు (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ఒకవేళ (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        ఆపు;
                                    }
                                }
                                Ast_కణుపు_DType* స్థిరరాశి substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), కణుపు_DType)
                                                   : ptdp->subDTypep();
                                ఒకవేళ (substp) గమ్యం_Replace.emplace_వెనుక(refp, substp);
                            });
                            కొరకు (స్వయంచాలక it = గమ్యం_Replace.rbegin(); it != గమ్యం_Replace.rend(); ++it) {
                                AstRefDType* స్థిరరాశి refp = it->మొదటి;
                                refp->replaceWith(it->రెండవ->clone_వృక్షం(అబద్ధం));
                                VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
                                any = నిజం;
                            }
                            ఒకవేళ (!any) ఆపు;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_కణుపు* np) {
                            ఒకవేళ (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = నిజం;
                                తిరిగి;
                            }
                            ఒకవేళ (Ast_Enum_అంశం_Ref* స్థిరరాశి అంశం_Refp = VN_CAST(np, Enum_అంశం_Ref)) {
                                ఒకవేళ (V3LinkDotIfaceCapture::కనుగొను_Owner_Module(అంశం_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = నిజం;
                                }
                                తిరిగి;
                            }
                            AstRefDType* స్థిరరాశి refp = VN_CAST(np, RefDType);
                            ఒకవేళ (!refp) తిరిగి;
                            ఒకవేళ (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = నిజం;
                            }
                            // A type declared in the template must not be widthed there
                            ఒకవేళ (V3LinkDotIfaceCapture::కనుగొను_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::కనుగొను_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = నిజం;
                            }
                        });
                    }
                    ఒకవేళ (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        ఒకవేళ (AstConst* స్థిరరాశి widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            ఒకవేళ (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_వెనుక();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_వృక్షం(), cloneVarp);
                }
                AstConst* స్థిరరాశి namingExprp = normedNamep ? normedNamep : exprp;
                ఒకవేళ (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_కణుపు* స్థిరరాశి pinExprp = pinp->exprp();
                    pinExprp->replaceWith(కొత్త AstConst{pinp->fileline(), AstConst::Widthed_విలువ{},
                                                       modvarp->వెడల్పు(), 0});
                    VL_DO_DANGLING(pinExprp->delete_వృక్షం(), pinExprp);
                } లేకపోతే ఒకవేళ (origp
                           && (exprp->same_వృక్షం(origp)
                               || (exprp->అంకె().వెడల్పు() == origp->అంకె().వెడల్పు()
                                   && Parameterized_Hier_ఖండాలు::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_వెడల్పు(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } లేకపోతే ఒకవేళ (namingExprp->అంకె().isDouble() || namingExprp->అంకె().isString()
                           || namingExprp->అంకె().isFourState()
                           || namingExprp->అంకె().వెడల్పు() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_విలువ_సంఖ్య(namingExprp));
                    any_overridesr = నిజం;
                } లేకపోతే {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->అంకె().ascii(అబద్ధం));
                    any_overridesr = నిజం;
                }
                ఒకవేళ (normedNamep) VL_DO_DANGLING(normedNamep->delete_వృక్షం(), normedNamep);
            }
        } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_కణుపు_DType* rawTypep = VN_CAST(pinp->exprp(), కణుపు_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            తార్కిక skip_వెడల్పు_For_Template_Struct = అబద్ధం;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_కణుపు_DType* స్థిరరాశి resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : శూన్యసూచిక;
                ఒకవేళ (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_కణుపు_Module* స్థిరరాశి ownerModp
                        = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    ఒకవేళ (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_వెడల్పు_For_Template_Struct = నిజం;
                        V3Stats::కూడు_Stat_కూడిక("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                ఒకవేళ (rawTypep && !skip_వెడల్పు_For_Template_Struct) V3_వెడల్పు::వెడల్పు_Params_Edit(rawTypep);
            }
            Ast_కణుపు_DType* exprp = rawTypep ? rawTypep->skip_Ref_గమ్యం_Non_Refp() : శూన్యసూచిక;
            స్థిరరాశి Ast_కణుపు_DType* origp = modvarp->skip_Ref_గమ్యం_Non_Refp();
            ఒకవేళ (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } లేకపోతే ఒకవేళ (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } లేకపోతే {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                ఒకవేళ (!skip_వెడల్పు_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), కణుపు_DType);
                    exprp = rawTypep->skip_Ref_గమ్యం_Non_Refp();
                }
                ఒకవేళ (!modvarp->fwdType().is_కణుపు_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                ఒకవేళ (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } లేకపోతే {
                    ఒకవేళ (!skip_వెడల్పు_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), కణుపు_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_గమ్యం_Non_Refp() : శూన్యసూచిక;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        ఒకవేళ (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } లేకపోతే {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    శూన్య cellInterfaceCleanup(AstPin* pinsp, Ast_కణుపు_Module* srcModp, string& longnamer,
                              తార్కిక& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        కొరకు (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            స్థిరరాశి AstVar* స్థిరరాశి modvarp = pinp->modVarp();
            ఒకవేళ (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                కొనసాగు;
            }
            ఒకవేళ (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* స్థిరరాశి పోర్టు_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి exprp = pinp->exprp();
                ఒకవేళ (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    కొనసాగు;
                }
                AstIfaceRefDType* pinIrefp = శూన్యసూచిక;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి vrp = VN_CAST(exprp, కణుపు_Var_Ref)) {
                    ఒకవేళ (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                ఒకవేళ (!pinIrefp && exprp) {
                    స్థిరరాశి Ast_కణుపు* basep = exprp;
                    అయితే (స్థిరరాశి Ast_కణుపు_Pre_Sel* స్థిరరాశి selp = VN_CAST(basep, కణుపు_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vrp = VN_CAST(basep, VarRef)) {
                        ఒకవేళ (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << పోర్టు_Irefp);

                ఒకవేళ (!పోర్టు_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } లేకపోతే ఒకవేళ (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } లేకపోతే {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    ఒకవేళ (పోర్టు_Irefp->iface_ద్వారా_Cellp() != pinIrefp->iface_ద్వారా_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_విలువ_సంఖ్య(pinIrefp));
                        any_overridesr = నిజం;
                        ifaceRefRefs.emplace_వెనుక(పోర్టు_Irefp, pinIrefp);
                        ఒకవేళ (పోర్టు_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && పోర్టు_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_కణుపు::prettyNameQ(పోర్టు_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_కణుపు::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    శూన్య storeOriginalParams(Ast_కణుపు_Module* స్థిరరాశి classp) {
        కొరకు (Ast_కణుపు* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_కణుపు* originalParamp = శూన్యసూచిక;
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isGParam()) originalParamp = varp->clonep();
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ఒకవేళ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ఒకవేళ (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    శూన్య generic_Interface_Var_అమరిక(స్థిరరాశి AstPin* స్థిరరాశి paramsp, స్థిరరాశి AstPin* స్థిరరాశి pinsp) {
        std::unordered_map<string, స్థిరరాశి AstPin*> paramspMap;
        కొరకు (స్థిరరాశి AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.చొప్పించు({pinp->name().substr(std::అక్షరశ్రేణి_పొడవు("__VGIfaceParam")), pinp});
            }
        }

        ఒకవేళ (paramspMap.ఖాళీలు()) తిరిగి;

        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstPin* స్థిరరాశి pinp = VN_CAST(nodep, Pin)) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = pinp->modVarp()) {
                    Ast_కణుపు_DType* dtypep = varp->బిడ్డ_DTypep()->elemDTypep();
                    ఒకవేళ (AstIfaceGenericDType* స్థిరరాశి ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        స్థిరరాశి స్వయంచాలక iter = paramspMap.కనుగొను(varp->name());
                        ఒకవేళ (iter == paramspMap.చివర()) కొనసాగు;
                        Ast_కణుపు* స్థిరరాశి backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_వెనుక();
                        స్థిరరాశి AstPin* స్థిరరాశి paramp = iter->రెండవ;
                        paramspMap.erase(iter);
                        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* స్థిరరాశి newIfacerefp = కొత్త AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        ఒకవేళ (స్వయంచాలక* స్థిరరాశి arrDtp = VN_CAST(backp, కణుపు_Array_DType)) {
                            arrDtp->బిడ్డ_DTypep(newIfacerefp);
                        } లేకపోతే ఒకవేళ (స్వయంచాలక* స్థిరరాశి arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->బిడ్డ_DTypep(newIfacerefp);
                        } లేకపోతే {
                            varp->బిడ్డ_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.నెట్టు_Deletep(ifaceGDTypep), ifaceGDTypep);
                        ఒకవేళ (paramspMap.ఖాళీలు()) తిరిగి;
                    }
                }
            }
        }

        UASSERT(paramspMap.ఖాళీలు(), "Not every generic interface implicit param is used");
    }

    శూన్య resolveDefaultParams(Ast_కణుపు* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = శూన్యసూచిక;
        AstPin* paramsp = శూన్యసూచిక;

        ఒకవేళ (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            ఒకవేళ (!classp) తిరిగి;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } లేకపోతే ఒకవేళ (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } లేకపోతే {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_సూచీ.clear();
        కొరకు (Ast_కణుపు* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                ఒకవేళ (!paramTypep->isGParam()) కొనసాగు;
                m_param_సూచీ.emplace(paramTypep, static_cast<పూర్ణాంకం>(m_classTypeParams.కొలత()));
                m_classTypeParams.emplace_వెనుక(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::సదిశ<Ast_కణుపు_DType*> defaultTypeNodes(m_classTypeParams.కొలత(), శూన్యసూచిక);
        కొరకు (కొలత_t i = 0; i < m_classTypeParams.కొలత(); ++i) {
            AstParamTypeDType* స్థిరరాశి paramTypep = m_classTypeParams[i].మొదటి;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* స్థిరరాశి reqDtp = VN_CAST(paramTypep->get_బిడ్డ_DTypep(), RequireDType);
            ఒకవేళ (!reqDtp) కొనసాగు;

            // If default is a reference to another param type, record dependency
            ఒకవేళ (AstRefDType* స్థిరరాశి refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                ఒకవేళ (AstParamTypeDType* స్థిరరాశి ఆధారం_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    స్వయంచాలక it = m_param_సూచీ.కనుగొను(ఆధారం_Paramp);
                    ఒకవేళ (it != m_param_సూచీ.చివర()) { m_classTypeParams[i].రెండవ = it->రెండవ; }
                    కొనసాగు;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(reqDtp->lhsp(), కణుపు_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::సదిశ<AstPin*> pins_By_సూచీ;
        pins_By_సూచీ.resize(m_classTypeParams.కొలత(), శూన్యసూచిక);
        కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* స్థిరరాశి typep = pinp->modPTypep();
            స్థిరరాశి స్వయంచాలక it = typep ? m_param_సూచీ.కనుగొను(typep) : m_param_సూచీ.చివర();
            ఒకవేళ (it != m_param_సూచీ.చివర()) {
                pins_By_సూచీ[it->రెండవ] = pinp;
            } లేకపోతే {
                pins_By_సూచీ.నెట్టు_వెనుక(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        కొరకు (తార్కిక resolvedSome = నిజం; resolvedSome;) {
            resolvedSome = అబద్ధం;
            కొరకు (కొలత_t paramIdx = 0; paramIdx < m_classTypeParams.కొలత(); paramIdx++) {
                ఒకవేళ (pins_By_సూచీ[paramIdx]) కొనసాగు;
                స్థిరరాశి పూర్ణాంకం ఆధారం_Param_Idx = m_classTypeParams[paramIdx].రెండవ;

                AstPin* newPinp = శూన్యసూచిక;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                ఒకవేళ (ఆధారం_Param_Idx >= 0 && pins_By_సూచీ[ఆధారం_Param_Idx]) {
                    newPinp = pins_By_సూచీ[ఆధారం_Param_Idx]->clone_వృక్షం(అబద్ధం);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                ఒకవేళ (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_కణుపు_DType* స్థిరరాశి dtypep = defaultTypeNodes[paramIdx];
                    newPinp = కొత్త AstPin{dtypep->fileline(), static_cast<పూర్ణాంకం>(paramIdx) + 1,
                                         "__paramNumber" + cvt_గమ్యం_Str(paramIdx + 1),
                                         dtypep->clone_వృక్షం(అబద్ధం)};
                }

                ఒకవేళ (newPinp) {
                    newPinp->name("__paramNumber" + cvt_గమ్యం_Str(paramIdx + 1));
                    newPinp->param(నిజం);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].మొదటి);
                    ఒకవేళ (classOrPackageRef) {
                        classOrPackageRef->కూడు_Paramsp(newPinp);
                    } లేకపోతే ఒకవేళ (classRefDType) {
                        classRefDType->కూడు_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_సూచీ[paramIdx] = newPinp;
                    ఒకవేళ (!paramsp) paramsp = newPinp;
                    resolvedSome = నిజం;
                }
            }
        }
    }

    Ast_కణుపు_Module* కణుపు_Deparam_Common(Ast_కణుపు* nodep, Ast_కణుపు_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, తార్కిక any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        ఒకవేళ (debug() >= 9 && paramsp) paramsp->dump_వృక్షం_And_తదుపరి(బయటకు, "-  cellparams: ");

        ఒకవేళ (srcModp->hier_ఖండం()) {
            longname = parameterized_Hier_ఖండం_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } లేకపోతే {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_సమితి<స్థిరరాశి AstPin*> overridingTypePins;
            కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (!pinp->exprp()) కొనసాగు;  // An empty override keeps the default
                ఒకవేళ (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    కొనసాగు;
                }
                AstVar* స్థిరరాశి modvarp = pinp->modVarp();
                ఒకవేళ (!modvarp || !modvarp->isGParam()) కొనసాగు;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                స్థిరరాశి Ast_నమూనా* స్థిరరాశి patternp = VN_CAST(pinp->exprp(), నమూనా);
                ఒకవేళ (patternp && !patternp->బిడ్డ_DTypep()) కొనసాగు;
                ఒకవేళ (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_విలువ(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (AstParamTypeDType* స్థిరరాశి modptp = pinp->modPTypep()) {
                    ఒకవేళ (!overridingTypePins.లెక్క(pinp)) కొనసాగు;
                    Ast_కణుపు_DType* స్థిరరాశి typep = VN_AS(pinp->exprp(), కణుపు_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_విలువ_సంఖ్య(typep->skip_Ref_గమ్యం_Non_Refp());
                    any_overrides = నిజం;
                    కొనసాగు;
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
        తార్కిక defaultsResolved = అబద్ధం;
        ఒకవేళ (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            కొరకు (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ఒకవేళ (pinp->modPTypep()) {
                    any_overrides = నిజం;
                    defaultsResolved = నిజం;
                    ఆపు;
                }
            }
        }
        ఒకవేళ (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.గమ్యం_String();
            any_overrides = నిజం;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_కణుపు_Module* newModp = శూన్యసూచిక;
        ఒకవేళ (m_hier_ఖండాలు.hier_Sub_నిరంతరం() && m_hier_ఖండాలు.is_Hier_ఖండం(srcModp->origName())) {
            Ast_కణుపు_Module* స్థిరరాశి paramedModp
                = m_hier_ఖండాలు.కనుగొను_By_Params(srcModp->origName(), paramsp, m_modp);
            ఒకవేళ (!paramedModp) తిరిగి శూన్యసూచిక;  // Unsupported, already reported
            paramedModp->dead(అబద్ధం);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } లేకపోతే ఒకవేళ (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            ఒకవేళ (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_కణుపు_Module* కణుపు_Copyp = srcModp->clone_వృక్షం(అబద్ధం);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << కణుపు_Copyp);
                m_deleter.నెట్టు_Deletep(కణుపు_Copyp);  // nodeCopyp used past here
                srcModp->user3p(కణుపు_Copyp);
                storeOriginalParams(కణుపు_Copyp);
            }
            newModp = srcModp;
        } లేకపోతే {
            స్థిరరాశి string newname
                = srcModp->hier_ఖండం() ? longname : moduleCalcName(srcModp, longname);
            స్థిరరాశి ModInfo* స్థిరరాశి modInfop
                = module_కనుగొను_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            ఒకవేళ (!modInfop) తిరిగి శూన్యసూచిక;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        స్థిరరాశి తార్కిక cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* స్థిరరాశి newClassp = VN_CAST(newModp, Class);
        ఒకవేళ (newClassp && !newClassp->user2()) m_specializedClassps.నెట్టు_వెనుక(newClassp);

        // Link source class to its specialized version for later relinking of method references
        ఒకవేళ (defaultsResolved) srcModp->user4p(newModp);

        కొరకు (స్వయంచాలక* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                ఒకవేళ (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_అమరిక(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        ఒకవేళ (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం(), paramsp);
        తిరిగి newModp;
    }

    Ast_కణుపు_Module* cellDeparam(AstCell* nodep, Ast_కణుపు_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_కణుపు_Module* స్థిరరాశి newModp = కణుపు_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        ఒకవేళ (!newModp) తిరిగి శూన్యసూచిక;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(అబద్ధం);
        తిరిగి newModp;
    }
    Ast_కణుపు_Module* ifaceRefDeparam(AstIfaceRefDType* స్థిరరాశి nodep, Ast_కణుపు_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        ఒకవేళ (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            స్థిరరాశి string ownerOrigName
                = ownerIfacep->origName().ఖాళీలు() ? ownerIfacep->name() : ownerIfacep->origName();
            స్థిరరాశి string srcOrigName
                = srcModp->origName().ఖాళీలు() ? srcModp->name() : srcModp->origName();
            string owner_పునాది_Name = ownerOrigName;
            స్థిరరాశి కొలత_t ownerPos = owner_పునాది_Name.కనుగొను("__");
            ఒకవేళ (ownerPos != string::npos) owner_పునాది_Name = owner_పునాది_Name.substr(0, ownerPos);
            string src_పునాది_Name = srcOrigName;
            స్థిరరాశి కొలత_t srcPos = src_పునాది_Name.కనుగొను("__");
            ఒకవేళ (srcPos != string::npos) src_పునాది_Name = src_పునాది_Name.substr(0, srcPos);

            ఒకవేళ (owner_పునాది_Name == src_పునాది_Name) {
                తార్కిక allOwnParams = నిజం;
                కొరకు (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    ఒకవేళ (AstRefDType* స్థిరరాశి refp = VN_CAST(pinp->exprp(), RefDType)) {
                        ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_కణుపు_Module* స్థిరరాశి ptdOwnerp
                                = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(ptdp);
                            ఒకవేళ (ptdOwnerp != m_modp) allOwnParams = అబద్ధం;
                        } లేకపోతే {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } లేకపోతే {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                ఒకవేళ (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::కూడు_Stat_కూడిక("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    ఒకవేళ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
                    తిరిగి ownerIfacep;
                }
            }
        }

        Ast_కణుపు_Module* స్థిరరాశి newModp
            = కణుపు_Deparam_Common(nodep, srcModp, nodep->paramsp(), శూన్యసూచిక, అబద్ధం);
        ఒకవేళ (!newModp) తిరిగి శూన్యసూచిక;
        nodep->ifacep(VN_AS(newModp, Iface));
        తిరిగి newModp;
    }
    Ast_కణుపు_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_కణుపు_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_కణుపు_Module* స్థిరరాశి newModp
            = కణుపు_Deparam_Common(nodep, srcModp, nodep->paramsp(), శూన్యసూచిక, అబద్ధం);
        ఒకవేళ (!newModp) తిరిగి శూన్యసూచిక;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* స్థిరరాశి refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* స్థిరరాశి newClassp = refDTypep ? VN_CAST(newModp, Class) : శూన్యసూచిక;
        ఒకవేళ (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            ఒకవేళ (AstTypedef* స్థిరరాశి typedefp
                = VN_CAST(m_memberMap.కనుగొను_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        తిరిగి newModp;
    }
    Ast_కణుపు_Module* classRefDeparam(AstClassRefDType* nodep, Ast_కణుపు_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_కణుపు_Module* స్థిరరాశి newModp
            = కణుపు_Deparam_Common(nodep, srcModp, nodep->paramsp(), శూన్యసూచిక, అబద్ధం);
        ఒకవేళ (!newModp) తిరిగి శూన్యసూచిక;
        AstClass* స్థిరరాశి newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        తిరిగి newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    తరగతి DeferredResolverVisitor final : బహిరంగ VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::సమితి<స్థిరరాశి Ast_కణుపు*> m_reachedDecls;

        తార్కిక మొదటి_వ్యాప్తి(స్థిరరాశి Ast_కణుపు* స్థిరరాశి declp) { తిరిగి m_reachedDecls.చొప్పించు(declp).రెండవ; }

        శూన్య iterateParamType(AstParamTypeDType* స్థిరరాశి paramTypep) {
            ఒకవేళ (మొదటి_వ్యాప్తి(paramTypep)) iterate(paramTypep);
        }
        శూన్య iterateTypedef(AstTypedef* స్థిరరాశి tdefp) {
            ఒకవేళ (మొదటి_వ్యాప్తి(tdefp)) iterate(tdefp->subDTypep());
        }

        శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
        శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_కణుపు* స్థిరరాశి targetp = nodep->classOrPackageNodep();
            ఒకవేళ (AstTypedef* స్థిరరాశి tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        శూన్య సందర్శించు(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_గమ్యం_Typedef(nodep);
        }
        శూన్య సందర్శించు(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* స్థిరరాశి tdefp = nodep->typedefp();
            ఒకవేళ (tdefp) {
                iterateTypedef(tdefp);
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        శూన్య సందర్శించు(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* స్థిరరాశి varp = nodep->varp();
            ఒకవేళ (మొదటి_వ్యాప్తి(varp)) {
                iterate(varp->subDTypep());
                స్థిరరాశి స్వయంచాలక& deferredVarps = v3Global.rootp()->deferredParamVarps();
                ఒకవేళ (varp->varType() == VVarType::LPARAM && deferredVarps.లెక్క(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    బహిరంగ:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_కణుపు* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

బహిరంగ:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    శూన్య retargetIfaceRefs(Ast_కణుపు_Module* మాతృ_Modp, స్థిరరాశి string& cellName) {
        Ast_కణుపు_Module* స్థిరరాశి correctModp
            = V3LinkDotIfaceCapture::follow_Cell_మార్గం(మాతృ_Modp, cellName);
        ఒకవేళ (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) తిరిగి;
        మాతృ_Modp->foreach([&](AstRefDType* refp) {
            స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = V3LinkDotIfaceCapture::capture_ట్యాగ్(refp);
            ఒకవేళ (!tagp || tagp->m_cell_మార్గం != cellName) తిరిగి;
            // Only retarget REFDTYPEs owned by parentModp.
            ఒకవేళ (V3LinkDotIfaceCapture::కనుగొను_Owner_Module(refp) != మాతృ_Modp) తిరిగి;
            ఒకవేళ (V3LinkDotIfaceCapture::retarget_Ref_గమ్యం_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    శూన్య resolveDeferredDotsReachableFrom(Ast_కణుపు* rootp, స్థిరరాశి Ast_కణుపు_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ఇది, rootp};
    }

    Ast_కణుపు_Module* కణుపు_Deparam(Ast_కణుపు* nodep, Ast_కణుపు_Module* srcModp, Ast_కణుపు_Module* modp,
                               స్థిరరాశి string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_కణుపు::కణుపు_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string కణుపు_Name = nodep->name();
        ఒకవేళ (AstIfaceRefDType* స్థిరరాశి ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            ఒకవేళ (కణుపు_Name.ఖాళీలు()) కణుపు_Name = ifaceRefp->cellName();
        }
        స్థిరరాశి string instanceName
            = కణుపు_Name.ఖాళీలు() ? someInstanceName : (someInstanceName + "." + కణుపు_Name);

        ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
            కొరకు (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* స్థిరరాశి nextp = VN_AS(pinp->nextp(), Pin);
                ఒకవేళ (!pinp->param_మార్గం().ఖాళీలు() && instanceName != pinp->param_మార్గం()
                    && !VString::చివర్లు_With(instanceName, "." + pinp->param_మార్గం())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_వెనుక()->delete_వృక్షం(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_కణుపు::కణుపు_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_కణుపు_Module* newModp = శూన్యసూచిక;
        ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } లేకపోతే ఒకవేళ (AstIfaceRefDType* స్థిరరాశి ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } లేకపోతే ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } లేకపోతే ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } లేకపోతే {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        ఒకవేళ (!newModp) తిరిగి శూన్యసూచిక;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_కణుపు::కణుపు_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        తిరిగి newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::సదిశ<AstClass*> takeSpecializedClassps() {
        std::సదిశ<AstClass*> తీసుకున్న;
        తీసుకున్న.మార్పిడి(m_specializedClassps);
        తిరిగి తీసుకున్న;
    }

    // CONSTRUCTORS
    స్పష్టమైన ParamProcessor(AstNetlist* nodep)
        : m_hier_ఖండాలు{v3Global.opt.hier_ఖండాలు(), nodep} {
        కొరకు (Ast_కణుపు_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), కణుపు_Module)) {
            m_all_Module_పేర్లు.చొప్పించు(modp->name());
        }
    }
    ~ParamProcessor() = అప్రమేయ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

తరగతి ClassRefUnlinkerVisitor final : బహిరంగ VNVisitor {

బహిరంగ:
    స్పష్టమైన ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        ఒకవేళ (nodep->paramsp()) {
            ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                ఒకవేళ (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* స్థిరరాశి dotp = VN_CAST(nodep->backp(), Dot);
                    ఒకవేళ (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_వెనుక());
                        VL_DO_DANGLING2(నెట్టు_Deletep(dotp), dotp, nodep);
                    } లేకపోతే {
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    }
                }
            }
        }
    }
    శూన్య సందర్శించు(AstClass* nodep) override {}  // don't iterate inside classes
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

తరగతి ParamClassRefDTypeRelinkVisitor final : బహిరంగ VNVisitor {

    AstClass* m_classp = శూన్యసూచిక;
    std::unordered_map<స్థిరరాశి AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_కణుపు_Module* m_ownerModp = శూన్యసూచిక;
    Ast_కణుపు_Module* m_ownerMapForp = శూన్యసూచిక;
    std::unordered_map<std::string, AstClass*> m_orig_Name_గమ్యం_Clone;
    std::unordered_సమితి<std::string> m_ambiguous;

    శూన్య ensureOwnerMap() {
        ఒకవేళ (m_ownerMapForp == m_ownerModp) తిరిగి;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_గమ్యం_Clone.clear();
        m_ambiguous.clear();
        ఒకవేళ (!m_ownerModp) తిరిగి;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        ఒకవేళ (AstClass* స్థిరరాశి ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ఒకవేళ (ownerClassp->hasGParam()) తిరిగి;
        }
        కొరకు (Ast_కణుపు* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* స్థిరరాశి tdp = VN_CAST(sp, Typedef);
            ఒకవేళ (!tdp) కొనసాగు;
            AstClassRefDType* స్థిరరాశి crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            ఒకవేళ (!crdtp || !crdtp->classp()) కొనసాగు;
            AstClass* స్థిరరాశి targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            ఒకవేళ (targetp->hasGParam()) కొనసాగు;
            స్థిరరాశి std::string origName
                = targetp->origName().ఖాళీలు() ? targetp->name() : targetp->origName();
            స్థిరరాశి స్వయంచాలక pair = m_orig_Name_గమ్యం_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            ఒకవేళ (!pair.రెండవ && pair.మొదటి->రెండవ != targetp) m_ambiguous.చొప్పించు(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    స్థిర string classOrigName(స్థిరరాశి AstClass* classp) {
        తిరిగి classp->origName().ఖాళీలు() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    స్థిర AstTypedef* కనుగొను_Typedef_With_Bases(AstClass* classp, స్థిరరాశి string& name) {
        కొరకు (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : శూన్యసూచిక) {
            ఒకవేళ (AstTypedef* స్థిరరాశి tdp = V3LinkDotIfaceCapture::కనుగొను_Typedef_లోపల_Module(cp, name)) {
                ఒకవేళ (tdp->subDTypep()) తిరిగి tdp;
            }
        }
        తిరిగి శూన్యసూచిక;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    శూన్య retargetRefDType(AstRefDType* refp) {
        ఒకవేళ (!m_ownerModp) తిరిగి;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        ఒకవేళ (AstClass* స్థిరరాశి ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ఒకవేళ (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                స్థిరరాశి AstClass* refClassp = శూన్యసూచిక;
                ఒకవేళ (AstTypedef* స్థిరరాశి tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::కనుగొను_Owner_Module(tdp), Class);
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassOrPackageRef* స్థిరరాశి classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                ఒకవేళ (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* స్థిరరాశి selfTdp = కనుగొను_Typedef_With_Bases(ownerClassp, refp->name());
                    ఒకవేళ (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::కనుగొను_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        తిరిగి;
                    }
                }
            }
        }

        AstTypedef* స్థిరరాశి oldTdp = refp->typedefp();
        ఒకవేళ (!oldTdp) తిరిగి;
        AstClass* స్థిరరాశి oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::కనుగొను_Owner_Module(oldTdp), Class);
        ఒకవేళ (!oldOwnerp) తిరిగి;

        ensureOwnerMap();
        ఒకవేళ (m_orig_Name_గమ్యం_Clone.ఖాళీలు()) తిరిగి;
        స్థిరరాశి std::string origName
            = oldOwnerp->origName().ఖాళీలు() ? oldOwnerp->name() : oldOwnerp->origName();
        ఒకవేళ (m_ambiguous.లెక్క(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            తిరిగి;
        }
        స్థిరరాశి స్వయంచాలక it = m_orig_Name_గమ్యం_Clone.కనుగొను(origName);
        ఒకవేళ (it == m_orig_Name_గమ్యం_Clone.చివర()) తిరిగి;
        AstClass* స్థిరరాశి correctClassp = it->రెండవ;
        ఒకవేళ (correctClassp == oldOwnerp) తిరిగి;
        AstTypedef* స్థిరరాశి newTdp
            = V3LinkDotIfaceCapture::కనుగొను_Typedef_లోపల_Module(correctClassp, refp->name());
        ఒకవేళ (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            తిరిగి;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        ఒకవేళ (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            తిరిగి;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

బహిరంగ:
    స్పష్టమైన ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        ఒకవేళ (AstParamTypeDType* స్థిరరాశి paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            స్థిరరాశి స్వయంచాలక it = m_paramTypeClassMap.కనుగొను(paramtypep);
            ఒకవేళ (it != m_paramTypeClassMap.చివర()) {
                AstClass* స్థిరరాశి origClassp = it->రెండవ;
                ఒకవేళ (origClassp->hasGParam() && !origClassp->user3p()) {
                    ఒకవేళ (AstClass* స్థిరరాశి parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        స్థిరరాశి string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = శూన్యసూచిక;
                        కొరకు (Ast_కణుపు* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                ఒకవేళ (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    ఆపు;
                                }
                            }
                        }
                        ఒకవేళ (newParamTypep) {
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
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

తరగతి ParamState final {
బహిరంగ:
    // STATE - across all visitors
    std::సదిశ<AstClass*> m_paramClasses;  // Parameterized classes
    std::సదిశ<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    ఉపయోగించి WQKey = std::pair<తార్కిక, పూర్ణాంకం>;
    std::multimap<WQKey, Ast_కణుపు_Module*> m_work_వరుస_తదుపరి;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_కణుపు_Module*, std::unordered_సమితి<Ast_కణుపు_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

తరగతి ParamVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    తార్కిక m_iterateModule = అబద్ధం;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<తార్కిక, Ast_కణుపు*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<స్థిరరాశి Ast_కణుపు*, std::string>
        m_gen_Hier_పేర్లు;  // Maps ast nodes to generated hierarchy names
    std::map<స్థిరరాశి AstRefDType*, తార్కిక>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    ఉపయోగించి VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<స్థిరరాశి Ast_కణుపు_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Module iterating
    std::unordered_సమితి<std::string> m_iface_పోర్టు_పేర్లు;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    తార్కిక m_mod_Iface_Refs_పూర్తి = అబద్ధం;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    తార్కిక m_లోపల_Generate_Cond = అబద్ధం;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    శూన్య processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_కణుపు_Module*> work_వరుస;
        m_generateHierName = "";
        m_iterateModule = నిజం;

        // Visit all cells under module, recursively
        అయితే (నిజం) {
            // Classes specialized since the last pass still need their bodies elaborated
            కొరకు (AstClass* స్థిరరాశి classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_వరుస_తదుపరి.emplace(ParamState::WQKey{నిజం, classp->level()}, classp);
            }
            ఒకవేళ (work_వరుస.ఖాళీలు()) std::మార్పిడి(work_వరుస, m_state.m_work_వరుస_తదుపరి);
            ఒకవేళ (work_వరుస.ఖాళీలు()) ఆపు;

            స్థిరరాశి స్వయంచాలక itm = work_వరుస.cbegin();
            Ast_కణుపు_Module* స్థిరరాశి modp = itm->రెండవ;
            work_వరుస.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            ఒకవేళ (!modp->user2_సమితి_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                ఒకవేళ (modp->someInstanceName().ఖాళీలు()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_పోర్టు_పేర్లు);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_పూర్తి);
                    m_modp = modp;
                    m_mod_Iface_Refs_పూర్తి = అబద్ధం;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            అయితే (!m_cellps.ఖాళీలు()) {
                స్థిరరాశి స్వయంచాలక itim = m_cellps.cbegin();
                Ast_కణుపు* స్థిరరాశి cellp = itim->రెండవ;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                స్థిరరాశి స్వయంచాలక genHierIt = m_gen_Hier_పేర్లు.కనుగొను(cellp);
                ఒకవేళ (genHierIt != m_gen_Hier_పేర్లు.చివర()) {
                    genHierName = std::move(genHierIt->రెండవ);
                    m_gen_Hier_పేర్లు.erase(genHierIt);
                }

                Ast_కణుపు_Module* srcModp = శూన్యసూచిక;
                ఒకవేళ (స్థిరరాశి AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    ఒకవేళ (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) కొనసాగు;
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } లేకపోతే {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                ఒకవేళ (!srcModp) కొనసాగు;

                // Update path
                స్థిరరాశి string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి newModp
                    = m_processor.కణుపు_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    ఒకవేళ (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        ఒకవేళ (V3LinkDotIfaceCapture::enabled()) {
                            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి modCellp = VN_CAST(cellp, Cell)) {
                                ఒకవేళ (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_వరుస.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].చొప్పించు(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    ఒకవేళ (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = అబద్ధం;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    స్థిర string get_Ref_పునాది_Name(స్థిరరాశి AstVarXRef* refp) {
        స్థిరరాశి string dotted = refp->dotted();
        ఒకవేళ (dotted.ఖాళీలు()) తిరిగి "";
        తిరిగి dotted.substr(0, dotted.కనుగొను('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    శూన్య logTemplateLeakRefs(Ast_కణుపు_Module* మాతృ_Modp, Ast_కణుపు_Module* templateModp,
                             స్థిరరాశి అక్షరం* stage, Ast_కణుపు* contextp) {
        ఒకవేళ (debug() < 9 || !మాతృ_Modp || !templateModp || !VN_IS(templateModp, Iface)) తిరిగి;
        // LCOV_EXCL_START  // Debug-only diagnostic
        పూర్ణాంకం leak_లెక్క = 0;
        స్థిరరాశి స్వయంచాలక ancestry_యొక్క = [](స్థిరరాశి Ast_కణుపు* nodep) {
            string ancestry;
            కొరకు (స్థిరరాశి Ast_కణుపు* curp = nodep; curp; curp = curp->backp()) {
                ఒకవేళ (!ancestry.ఖాళీలు()) ancestry += "<-";
                ancestry += curp->typeName();
                ఒకవేళ (VN_IS(curp, కణుపు_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_పట్టిక)) {
                    ఆపు;
                }
            }
            తిరిగి ancestry;
        };

        మాతృ_Modp->foreach([&](AstRefDType* refp) {
            ఒకవేళ (refp->typedefp()) {
                Ast_కణుపు_Module* స్థిరరాశి tdOwnerp
                    = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(refp->typedefp());
                ఒకవేళ (tdOwnerp == templateModp) {
                    ++leak_లెక్క;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << మాతృ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_కణుపు::కణుపు_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_యొక్క(refp));
                }
            }
            ఒకవేళ (refp->refDTypep()) {
                Ast_కణుపు_Module* స్థిరరాశి rdOwnerp
                    = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(refp->refDTypep());
                ఒకవేళ (rdOwnerp == templateModp) {
                    ++leak_లెక్క;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << మాతృ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_కణుపు::కణుపు_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_యొక్క(refp));
                }
            }
        });

        మాతృ_Modp->foreach([&](AstVarRef* varrefp) {
            ఒకవేళ (!varrefp->varp()) తిరిగి;
            Ast_కణుపు_Module* స్థిరరాశి varOwnerp
                = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(varrefp->varp());
            ఒకవేళ (varOwnerp != templateModp) తిరిగి;
            ++leak_లెక్క;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << మాతృ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_కణుపు::కణుపు_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_యొక్క(varrefp));
        });

        ఒకవేళ (leak_లెక్క > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << మాతృ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_లెక్క);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    శూన్య checkParamNotHierRecurse(Ast_కణుపు* nodep, తార్కిక underTypeQuery = అబద్ధం) {
        కొరకు (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            స్థిరరాశి Ast_Attr_యొక్క* స్థిరరాశి attrp = VN_CAST(nodep, Attr_యొక్క);
            స్థిరరాశి తార్కిక బిడ్డ_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            ఒకవేళ (స్థిరరాశి AstVarXRef* స్థిరరాశి refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                తార్కిక isIfaceRef = అబద్ధం;
                ఒకవేళ (refp->varp() && refp->varp()->isIfaceParam()) {
                    స్థిరరాశి string refname = get_Ref_పునాది_Name(refp);
                    isIfaceRef
                        = !refname.ఖాళీలు()
                          && (m_iface_పోర్టు_పేర్లు.లెక్క(refname) || m_ifaceInstCells.లెక్క(refname));
                }
                ఒకవేళ (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_FTask_Ref* స్థిరరాశి refp = VN_CAST(nodep, కణుపు_FTask_Ref)) {
                ఒకవేళ (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), బిడ్డ_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), బిడ్డ_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), బిడ్డ_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), బిడ్డ_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    శూన్య specializeNestedIfaceCells(Ast_కణుపు_Module* ifaceModp) {
        కొరకు (Ast_కణుపు* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* స్థిరరాశి nestedCellp = VN_CAST(stmtp, Cell);
            ఒకవేళ (!nestedCellp) కొనసాగు;
            ఒకవేళ (!VN_IS(nestedCellp->modp(), Iface)) కొనసాగు;
            ఒకవేళ (!nestedCellp->paramsp()) కొనసాగు;

            Ast_కణుపు_Module* స్థిరరాశి nestedSrcModp = nestedCellp->modp();
            ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి nestedNewModp = m_processor.కణుపు_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                ఒకవేళ (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    కొరకు (Ast_కణుపు* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(sp, Var)) {
                            ఒకవేళ (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    ఒకవేళ (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    తార్కిక cellParamsReferenceIfacePorts(AstCell* cellp) {
        ఒకవేళ (!cellp->paramsp()) తిరిగి అబద్ధం;

        కొరకు (Ast_కణుపు* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (AstPin* స్థిరరాశి pinp = VN_CAST(nodep, Pin)) {
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి exprp = pinp->exprp()) {
                    ఒకవేళ (స్థిరరాశి AstVarXRef* స్థిరరాశి refp = VN_CAST(exprp, VarXRef)) {
                        స్థిరరాశి string refname = get_Ref_పునాది_Name(refp);
                        ఒకవేళ (!refname.ఖాళీలు()
                            && (m_iface_పోర్టు_పేర్లు.లెక్క(refname)
                                || m_ifaceInstCells.లెక్క(refname)))
                            తిరిగి నిజం;
                    }
                }
            }
        }
        తిరిగి అబద్ధం;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    శూన్య expand_Iface_Array_పోర్టు_Dimensions(AstVar* portp, Ast_కణుపు_DType* dtypep,
                                        స్థిరరాశి std::string& suffix) {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        ఒకవేళ (!arrp) {
            AstVar* స్థిరరాశి varp = portp->clone_వృక్షం(అబద్ధం);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి oldp = varp->బిడ్డ_DTypep()) {
                VL_DO_DANGLING(నెట్టు_Deletep(oldp->unlink_Fr_వెనుక()), oldp);
            }
            varp->dtypep(శూన్యసూచిక);
            varp->బిడ్డ_DTypep(portp->subDTypep()->elemDTypep()->clone_వృక్షం(అబద్ధం));
            portp->కూడు_తదుపరి_ఇక్కడ(varp);
            తిరిగి;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        కొరకు (స్థిరరాశి పూర్ణాంకం n : arrp->declRange().seq_కుడి_గమ్యం_ఎడమ()) {
            స్థిరరాశి std::string s = suffix + "__BRA__" + Ast_కణుపు::encode_సంఖ్య(n) + "__KET__";
            expand_Iface_Array_పోర్టు_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    శూన్య expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   స్థిరరాశి AstRange* rangep, స్థిరరాశి std::string& suffix, పూర్ణాంకం idx) {
        // Base case: insert the element when no dimensions left
        ఒకవేళ (!rangep) {
            AstCell* స్థిరరాశి elemp = arrayedCellp->clone_వృక్షం(అబద్ధం);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->కూడు_తదుపరి_ఇక్కడ(elemp);
            ఒకవేళ (ifaceVarp) {
                AstIfaceRefDType* స్థిరరాశి irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_వృక్షం(అబద్ధం);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* స్థిరరాశి varp = ifaceVarp->clone_వృక్షం(అబద్ధం);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి oldp = varp->బిడ్డ_DTypep()) {
                    VL_DO_DANGLING(నెట్టు_Deletep(oldp->unlink_Fr_వెనుక()), oldp);
                }
                varp->dtypep(శూన్యసూచిక);
                varp->బిడ్డ_DTypep(irefp);
                ifaceVarp->కూడు_తదుపరి_ఇక్కడ(varp);
            }
            తిరిగి;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        స్థిరరాశి AstRange* స్థిరరాశి subRangep = VN_AS(rangep->nextp(), Range);
        కొరకు (స్థిరరాశి పూర్ణాంకం n : rangep->seq_కుడి_గమ్యం_ఎడమ()) {
            స్థిరరాశి std::string s = suffix + "__BRA__" + Ast_కణుపు::encode_సంఖ్య(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    శూన్య సందర్శించు_Cell_Or_Class_Ref(Ast_కణుపు* nodep, తార్కిక isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_పేర్లు.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        ఒకవేళ (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell);
            ఒకవేళ (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_కణుపు_Module* స్థిరరాశి srcModp = cellp->modp();
                Ast_కణుపు_Module* స్థిరరాశి newModp
                    = m_processor.కణుపు_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                ఒకవేళ (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                ఒకవేళ (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        ఒకవేళ (!m_లోపల_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->recursiveClone()) nodep->dead(నిజం);  // Fake, made for recursive elimination
        ఒకవేళ (nodep->dead()) తిరిగి;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            ఒకవేళ (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.నెట్టు_వెనుక(classp);
                తిరిగి;
            }
        }

        ఒకవేళ (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_వరుస_తదుపరి.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            తిరిగి;
        }

        // Start traversal at root-like things
        ఒకవేళ (nodep->is_శిఖరం()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_వరుస_తదుపరి.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    తార్కిక isCircularType(స్థిరరాశి AstRefDType* nodep) {
        స్థిరరాశి స్వయంచాలక iter = m_isCircular.emplace(nodep, నిజం);
        ఒకవేళ (!iter.రెండవ) తిరిగి iter.మొదటి->రెండవ;
        ఒకవేళ (స్థిరరాశి AstRefDType* స్థిరరాశి subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            స్థిరరాశి తార్కిక ret = isCircularType(subDTypep);
            iter.మొదటి->రెండవ = ret;
            తిరిగి ret;
        }
        iter.మొదటి->రెండవ = అబద్ధం;
        తిరిగి అబద్ధం;
    }

    శూన్య సందర్శించు(AstRefDType* nodep) override {
        స్థిరరాశి తార్కిక isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        ఒకవేళ (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        ఒకవేళ (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } లేకపోతే ఒకవేళ (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_సమితి_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        ఒకవేళ (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            కొరకు (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_వెడల్పు::వెడల్పు_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* స్థిరరాశి ifaceVarp = [&]() -> AstVar* {
                ఒకవేళ (!VN_IS(nodep->modp(), Iface)) తిరిగి శూన్యసూచిక;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* స్థిరరాశి varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                తిరిగి varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            ఒకవేళ (ifaceVarp) {
                ifaceVarp->is_Iface_Array_విభజించు(నిజం);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* స్థిరరాశి firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* స్థిరరాశి irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            తిరిగి;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        ఒకవేళ (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        సందర్శించు_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_సమితి_Once();
        ఒకవేళ (nodep->ifacep()) సందర్శించు_Cell_Or_Class_Ref(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_సమితి_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        సందర్శించు_Cell_Or_Class_Ref(nodep, అబద్ధం);
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        ఒకవేళ (!VN_IS(nodep->classOrPackageNodep(), Typedef)) సందర్శించు_Cell_Or_Class_Ref(nodep, అబద్ధం);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    స్థిర శూన్య gatherVars(Ast_కణుపు* stmtsp, తార్కిక (*matchp)(స్థిరరాశి AstVar*), VarsByName& varsr) {
        కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                ఒకవేళ (matchp(varp)) varsr.emplace(varp->name(), varp);
            } లేకపోతే ఒకవేళ (Ast_Gen_ఖండం* స్థిరరాశి genp = VN_CAST(nodep, Gen_ఖండం)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    స్థిరరాశి VarsByName& modIfaceRefs() {
        ఒకవేళ (!m_mod_Iface_Refs_పూర్తి) {
            gatherVars(
                m_modp->stmtsp(), [](స్థిరరాశి AstVar* varp) { తిరిగి varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_పూర్తి = నిజం;
        }
        తిరిగి m_modIfaceRefs;
    }
    స్థిరరాశి VarsByName& ifaceParams(Ast_కణుపు_Module* ifacep) {
        స్థిరరాశి స్వయంచాలక pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        ఒకవేళ (pair.రెండవ) {
            gatherVars(
                ifacep->stmtsp(), [](స్థిరరాశి AstVar* varp) { తిరిగి varp->isParam(); },
                pair.మొదటి->రెండవ);
        }
        తిరిగి pair.మొదటి->రెండవ;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_కణుపు* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* స్థిరరాశి lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* స్థిరరాశి rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        ఒకవేళ (!lhsp || !rhsp) తిరిగి శూన్యసూచిక;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        ఒకవేళ (VL_UNCOVERABLE(!m_modp)) తిరిగి శూన్యసూచిక;

        స్థిరరాశి స్వయంచాలక& refVars = modIfaceRefs();
        స్థిరరాశి స్వయంచాలక ifaceVarIt = refVars.కనుగొను(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        ఒకవేళ (VL_UNCOVERABLE(ifaceVarIt == refVars.చివర())) తిరిగి శూన్యసూచిక;
        AstVar* స్థిరరాశి ifaceVarp = ifaceVarIt->రెండవ;
        AstIfaceRefDType* స్థిరరాశి ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        ఒకవేళ (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) తిరిగి శూన్యసూచిక;

        స్థిరరాశి స్వయంచాలక& ifaceVars = ifaceParams(ifacerefp->ifacep());
        స్థిరరాశి స్వయంచాలక లక్ష్యం_Var_It = ifaceVars.కనుగొను(rhsp->name());
        ఒకవేళ (లక్ష్యం_Var_It == ifaceVars.చివర()) తిరిగి శూన్యసూచిక;
        AstVar* స్థిరరాశి లక్ష్యం_Varp = లక్ష్యం_Var_It->రెండవ;
        // processWorkQ() visits interface cells first, so the interface is already constified
        ఒకవేళ (VL_UNCOVERABLE(!VN_IS(లక్ష్యం_Varp->valuep(), Const))) iterate(లక్ష్యం_Varp);
        ఒకవేళ (VL_UNCOVERABLE(!లక్ష్యం_Varp->valuep() || !VN_IS(లక్ష్యం_Varp->valuep(), Const))) {
            తిరిగి శూన్యసూచిక;  // LCOV_EXCL_LINE
        }
        తిరిగి లక్ష్యం_Varp->valuep()->clone_వృక్షం(అబద్ధం);
    }
    శూన్య resolve_Generic_Iface_Dots_లోపల(Ast_కణుపు* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::సదిశ<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.నెట్టు_వెనుక(dotp); });
        కొరకు (AstDot* స్థిరరాశి dotp : dotps) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_వృక్షం(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (nodep->user2_సమితి_Once()) తిరిగి;  // Process once
        // Interface array port: split it into its elements
        ఒకవేళ (nodep->isIfaceRef() && !nodep->is_Iface_మాతృ()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_వెడల్పు::వెడల్పు_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_పోర్టు_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_విభజించు(నిజం);
        }
        // Build cache of interface port names as we encounter them
        ఒకవేళ (nodep->isIfaceRef()) { m_iface_పోర్టు_పేర్లు.చొప్పించు(nodep->name()); }
        iterateChildren(nodep);
        ఒకవేళ (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            ఒకవేళ (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } లేకపోతే ఒకవేళ (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_లోపల(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                తార్కిక hasUnresolvedLparamXRef = అబద్ధం;
                nodep->valuep()->foreach([&](స్థిరరాశి AstVarXRef* xrefp) {
                    ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = xrefp->varp()) {
                        ఒకవేళ (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = నిజం;
                        }
                    }
                });
                ఒకవేళ (hasUnresolvedLparamXRef) తిరిగి;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                స్థిరరాశి తార్కిక hasDot = nodep->valuep()->exists([](AstDot*) { తిరిగి నిజం; });
                తార్కిక refsDeferred = అబద్ధం;
                ఒకవేళ (!hasDot) {
                    స్థిరరాశి స్వయంచాలక& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](స్థిరరాశి AstVarRef* refp) {
                        ఒకవేళ (refsDeferred) తిరిగి;
                        AstVar* స్థిరరాశి refVarp = refp->varp();
                        ఒకవేళ (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.లెక్క(refVarp)) {
                            refsDeferred = నిజం;
                        }
                    });
                }
                ఒకవేళ (hasDot || refsDeferred) {
                    v3Global.rootp()->నెట్టు_Deferred_Param_Varp(nodep);
                    తిరిగి;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        ఒకవేళ (nodep->varp()) iterate(nodep->varp());
    }
    తార్కిక ifaceParamReplace(AstVarXRef* nodep, Ast_కణుపు* candp) {
        కొరకు (; candp; candp = candp->nextp()) {
            ఒకవేళ (nodep->name() == candp->name()) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    ఒకవేళ (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    తిరిగి నిజం;
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstPin* స్థిరరాశి pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_వృక్షం(అబద్ధం));
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి నిజం;
                }
            }
        }
        తిరిగి అబద్ధం;
    }

    శూన్య deparamIfaceCellNow(AstCell* cellp) {
        ఒకవేళ (!cellp->paramsp()) తిరిగి;
        ఒకవేళ (!VN_IS(cellp->modp(), Iface)) తిరిగి;
        Ast_కణుపు_Module* స్థిరరాశి srcModp = cellp->modp();
        Ast_కణుపు_Module* స్థిరరాశి newModp
            = m_processor.కణుపు_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        ఒకవేళ (newModp && newModp != srcModp) {
            ఒకవేళ (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (nodep->కలిగియుంది_Gen_ఖండం()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(శూన్యసూచిక);
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        ఒకవేళ (nodep->కలిగియుంది_Gen_ఖండం()) nodep->itemp(శూన్యసూచిక);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        ఒకవేళ (nodep->కలిగియుంది_Gen_ఖండం()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(శూన్యసూచిక);
            తిరిగి;
        }
        // Parameters of an interface array are accessed through its first element
        ఒకవేళ (nodep->varp() && nodep->varp()->isParam()) {
            స్థిరరాశి స్వయంచాలక it = m_ifaceInstCells.కనుగొను(nodep->dotted());
            ఒకవేళ (it != m_ifaceInstCells.చివర()) nodep->dotted(it->రెండవ->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        స్థిరరాశి string dotted = nodep->dotted();
        ఒకవేళ (!dotted.ఖాళీలు() && nodep->varp() && nodep->varp()->isParam()) {
            స్థిరరాశి Ast_కణుపు* backp = nodep;
            అయితే ((backp = backp->backp())) {
                ఒకవేళ (VN_IS(backp, కణుపు_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    ఆపు;
                }
                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(backp, Var)) {
                    ఒకవేళ (!varp->isIfaceRef()) కొనసాగు;
                    // Through all dimensions of an array
                    స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    ఒకవేళ (!ifacerefp) కొనసాగు;
                    // Interfaces passed in on the port map have ifaces
                    ఒకవేళ (స్థిరరాశి AstIface* స్థిరరాశి ifacep = ifacerefp->ifacep()) {
                        ఒకవేళ (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            ఒకవేళ (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                తిరిగి;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    లేకపోతే ఒకవేళ (AstCell* స్థిరరాశి cellp = ifacerefp->cellp()) {
                        ఒకవేళ (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            ఒకవేళ (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                తిరిగి;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            స్థిరరాశి స్వయంచాలక ifaceCellIt = m_ifaceInstCells.కనుగొను(dotted);
            ఒకవేళ (ifaceCellIt != m_ifaceInstCells.చివర()) {
                AstCell* స్థిరరాశి cellp = ifaceCellIt->రెండవ;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    శూన్య సందర్శించు(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        స్థిరరాశి AstClass* lhsClassp = శూన్యసూచిక;
        స్థిరరాశి AstClassOrPackageRef* స్థిరరాశి classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        ఒకవేళ (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_కణుపు* rhsDefp = శూన్యసూచిక;
        AstClassOrPackageRef* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        ఒకవేళ (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        ఒకవేళ (lhsClassp && rhsDefp) {
            m_state.m_dots.నెట్టు_వెనుక(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } లేకపోతే {
            iterate(nodep->rhsp());
        }
    }

    శూన్య సందర్శించు(AstUnlinkedRef* nodep) override {
        AstVarXRef* స్థిరరాశి varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_కణుపు_FTask_Ref* స్థిరరాశి taskrefp = VN_CAST(nodep->refp(), కణుపు_FTask_Ref);
        Ast_Enum_అంశం_Ref* స్థిరరాశి enumrefp = VN_CAST(nodep->refp(), Enum_అంశం_Ref);
        ఒకవేళ (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } లేకపోతే ఒకవేళ (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } లేకపోతే ఒకవేళ (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } లేకపోతే {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            తిరిగి;
        }
        iterate(nodep->cellrefp());

        ఒకవేళ (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } లేకపోతే ఒకవేళ (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } లేకపోతే {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        కొరకు (Ast_కణుపు *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::సదిశ<పూర్ణాంకం> indices;
        కొరకు (Ast_కణుపు* s = nodep->selp(); s; s = s->nextp()) {
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(s, Const);
            ఒకవేళ (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                తిరిగి;
            }
            indices.నెట్టు_వెనుక(constp->గమ్యం_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        స్థిరరాశి string viftopSuffix = "__Viftop";
        స్థిరరాశి string పునాది_Name
            = VString::చివర్లు_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().కొలత() - viftopSuffix.కొలత())
                  : nodep->name();
        స్థిరరాశి string placeholder = "__BRA__??__KET__";
        కొలత_t pos = m_unlinkedTxt.కనుగొను(పునాది_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        ఒకవేళ (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            తిరిగి;
        }
        pos += పునాది_Name.నిడివి();
        కొరకు (పూర్ణాంకం idx : indices) {
            స్థిరరాశి string replacement = "__BRA__" + Ast_కణుపు::encode_సంఖ్య(idx) + "__KET__";
            ఒకవేళ (m_unlinkedTxt.compare(pos, placeholder.నిడివి(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                తిరిగి;
            }
            m_unlinkedTxt.replace(pos, placeholder.నిడివి(), replacement);
            pos += replacement.నిడివి();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    శూన్య iterateGenerateCond(Ast_కణుపు* nodep) {
        VL_RESTORER(m_లోపల_Generate_Cond);
        m_లోపల_Generate_Cond = నిజం;
        iterate_And_తదుపరి_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    శూన్య సందర్శించు(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_వెడల్పు::వెడల్పు_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->condp(), Const)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_వెనుక_With_తదుపరి();
                nodep->replaceWith(keepp);
            } లేకపోతే {
                nodep->unlink_Fr_వెనుక();
            }
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            // Normal edit rules will now recurse the replacement
        } లేకపోతే {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        ఒకవేళ (AstGenFor* స్థిరరాశి forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_వెడల్పు::వెడల్పు_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            స్థిరరాశి string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
                nodep->కూడు_తదుపరి_ఇక్కడ(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } లేకపోతే {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    శూన్య సందర్శించు(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        తార్కిక తగిలింది = అబద్ధం;
        Ast_కణుపు* keepp = శూన్యసూచిక;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        కొరకు (Ast_Gen_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_అంశం)) {
            కొరకు (Ast_కణుపు* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_వెడల్పు::వెడల్పు_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        స్థిరరాశి AstConst* స్థిరరాశి exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        కొరకు (Ast_Gen_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_అంశం)) {
            కొరకు (Ast_కణుపు* ep = itemp->condsp(); ep;) {
                Ast_కణుపు* స్థిరరాశి nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        కొరకు (Ast_Gen_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_అంశం)) {
            ఒకవేళ (!itemp->isDefault()) {
                కొరకు (Ast_కణుపు* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి ccondp = VN_CAST(ep, Const)) {
                        V3_సంఖ్య సరిపోలిక{nodep, 1};
                        సరిపోలిక.opEq(ccondp->అంకె(), exprp->అంకె());
                        ఒకవేళ (!తగిలింది && సరిపోలిక.isNeqZero()) {
                            తగిలింది = నిజం;
                            keepp = itemp->itemsp();
                        }
                    } లేకపోతే {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        కొరకు (Ast_Gen_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_అంశం)) {
            ఒకవేళ (itemp->isDefault()) {
                ఒకవేళ (!తగిలింది) {
                    తగిలింది = నిజం;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        ఒకవేళ (keepp) {
            keepp->unlink_Fr_వెనుక_With_తదుపరి();
            nodep->replaceWith(keepp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = అప్రమేయ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

తరగతి Param_శిఖరం final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    శూన్య relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        కొరకు (AstDot* స్థిరరాశి dotp : m_state.m_dots) {
            స్థిరరాశి AstClassOrPackageRef* స్థిరరాశి classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            స్థిరరాశి AstClass* స్థిరరాశి lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* స్థిరరాశి rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            కొరకు (స్వయంచాలక* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    ఆపు;
                }
            }
        }
    }

    శూన్య fixLevel(Ast_కణుపు_Module* modp) {
        // Fix up level of module, based on who instantiates it
        ఒకవేళ (modp->user1_సమితి_Once()) తిరిగి;  // Already fixed
        ఒకవేళ (m_state.m_parentps[modp].ఖాళీలు()) తిరిగి;  // Leave top levels alone
        పూర్ణాంకం max_మాతృ_Level = 0;
        కొరకు (Ast_కణుపు_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_మాతృ_Level = std::max(max_మాతృ_Level, parentp->level());
        }
        ఒకవేళ (modp->level() <= max_మాతృ_Level) modp->level(max_మాతృ_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->లోతు(modp->level());
    }

    శూన్య resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::సదిశ<Ast_కణుపు_Module*> modps;
        కొరకు (Ast_కణుపు_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), కణుపు_Module);
            modp->unlink_Fr_వెనుక();
            modps.నెట్టు_వెనుక(modp);
        }

        // Fix-up levels
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        కొరకు (Ast_కణుపు_Module* స్థిరరాశి modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_క్రమీకరించు(modps.begin(), modps.చివర(),
                         [](స్థిరరాశి Ast_కణుపు_Module* ap, స్థిరరాశి Ast_కణుపు_Module* bp) {
                             తిరిగి ap->level() < bp->level();
                         });

        // Re-insert modules
        కొరకు (Ast_కణుపు_Module* స్థిరరాశి modp : modps) netlistp->కూడు_Modulesp(modp);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన Param_శిఖరం(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        స్థిరరాశి ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        స్థిరరాశి ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_కణుపు_FTask_Ref* ftaskrefp) {
            Ast_కణుపు_FTask* ftaskp = ftaskrefp->taskp();
            ఒకవేళ (!ftaskp || !ftaskp->class_పద్ధతి()) తిరిగి;
            స్థిరరాశి std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            స్థిరరాశి స్వయంచాలక ancestor_Class_యొక్క = [](Ast_కణుపు* startp) -> AstClass* {
                కొరకు (Ast_కణుపు* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    ఒకవేళ (AstClass* స్థిరరాశి cp = VN_CAST(np, Class)) తిరిగి cp;
                }
                తిరిగి శూన్యసూచిక;
            };
            AstClass* refClassp = ancestor_Class_యొక్క(ftaskrefp);
            ఒకవేళ (refClassp == ftaskrefp->classOrPackagep())
                తిరిగి;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_యొక్క(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            ఒకవేళ (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                తిరిగి;
            }
            ఒకవేళ (classp->user3p()) తిరిగి;  // will not get removed, no need to relink
            AstClass* స్థిరరాశి parametrizedClassp = VN_CAST(classp->user4p(), Class);
            ఒకవేళ (!parametrizedClassp) తిరిగి;
            Ast_కణుపు_FTask* newFuncp = శూన్యసూచిక;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_కణుపు_FTask* ftaskp) {
                ఒకవేళ (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    తిరిగి నిజం;
                }
                తిరిగి అబద్ధం;
            });
            ఒకవేళ (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        స్థిరరాశి ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_సమితి<AstClass*> removedClassps;
        కొరకు (AstClass* స్థిరరాశి classp : m_state.m_paramClasses) {
            ఒకవేళ (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                ఒకవేళ (V3Error::error_లెక్క()) కొనసాగు;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(నెట్టు_Deletep(classp->unlink_Fr_వెనుక()), classp);
            } లేకపోతే {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(అబద్ధం);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_కణుపు::user3_Clear_వృక్షం();
        కొరకు (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_కణుపు* స్థిరరాశి nodep) { nodep->user3(నిజం); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_కణుపు* స్థిరరాశి nodep) {
            nodep->foreachLink([&](Ast_కణుపు** స్థిరరాశి linkpp, స్థిరరాశి అక్షరం* namep) {
                ఒకవేళ (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = శూన్యసూచిక;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_శిఖరం() = అప్రమేయ;
    VL_UNCOPYABLE(Param_శిఖరం);
};

//######################################################################
// Param class functions

శూన్య V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    ఒకవేళ (dump_వృక్షం_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_శిఖరం{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    ఒకవేళ (dump_వృక్షం_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_వృక్షం("param", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    కొరకు (AstVar* స్థిరరాశి varp : rootp->deferredParamVarps()) {
        ఒకవేళ (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
