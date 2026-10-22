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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Param.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Case.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDotIfaceCapture.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Simulate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unroll.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdlib>
#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ਸਥਿਰ AstClassRefDType* class_Ref_DType_ਦਾ_ਗੰਢ(Ast_ਗੰਢ* nodep) {
    ਜਦੋਂ (AstTypedef* ਅਚਲ tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(nodep, ਗੰਢ_DType);
    ਵਾਪਸ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : ਖਾਲੀ_ਸੂਚਕ;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

ਵਰਗ Parameterized_Hier_ਖੰਡਾਂ final {
    ਵਰਤੋਂ Hier_ਖੰਡ_Opts_By_Orig_Name = std::multimap<std::string, ਅਚਲ V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option*>;
    ਵਰਤੋਂ HierMapIt = Hier_ਖੰਡ_Opts_By_Orig_Name::const_iterator;
    ਵਰਤੋਂ Hier_ਖੰਡ_Mod_Map = std::map<ਅਚਲ std::string, Ast_ਗੰਢ_Module*>;
    ਵਰਤੋਂ ParamConstMap = std::map<ਅਚਲ std::string, std::unique_ptr<AstConst>>;
    ਵਰਤੋਂ GParamsMap = std::map<ਅਚਲ std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ਅਚਲ ਤਾਰਕਿਕ m_hier_Sub_ਲੜੀਵਾਰ;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_ਖੰਡ_Opts_By_Orig_Name m_hier_ਖੰਡ_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_ਖੰਡ_Mod_Map m_hier_ਖੰਡ_Mod;
    // Overridden parameters of the hierarchical block
    std::map<ਅਚਲ V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ਅਚਲ std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_untypedParams;

    // METHODS

ਜਨਤਕ:
    Parameterized_Hier_ਖੰਡਾਂ(ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_ਲੜੀਵਾਰ{!v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ() || v3Global.opt.hier_ਬੱਚਾ()} {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& hierOpt : hierOpts) {
            m_hier_ਖੰਡ_Opts_By_Orig_Name.emplace(hierOpt.ਦੂਜਾ.origName(), &hierOpt.ਦੂਜਾ);
            ਅਚਲ V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option::ParamStrMap& params = hierOpt.ਦੂਜਾ.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.ਦੂਜਾ];
            ਲਈ (V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.ਸਿਰਾ(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::built_ਵਿੱਚ_Filename()}, pIt->ਦੂਜਾ)};
                UASSERT(constp, pIt->ਦੂਜਾ << " is not a valid parameter literal");
                ਅਚਲ ਤਾਰਕਿਕ inserted = consts.emplace(pIt->ਪਹਿਲਾ, std::move(constp)).ਦੂਜਾ;
                UASSERT(inserted, pIt->ਪਹਿਲਾ << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.ਪਾਓ({hierOpt.ਦੂਜਾ.origName(), {}});
        }
        ਲਈ (Ast_ਗੰਢ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
            ਜੇ (hierOpts.ਲੱਭੋ(modp->prettyName()) != hierOpts.ਸਿਰਾ()) {
                m_hier_ਖੰਡ_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ਅਚਲ string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ਅਚਲ ਸਵੈਚਾਲਿਤ defParamIt = m_modParams.ਲੱਭੋ(actualModName);
            ਜੇ (defParamIt != m_modParams.ਸਿਰਾ()) {
                // modp is the original of parameterized hierarchical block
                ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                        ਜੇ (varp->isGParam()) defParamIt->ਦੂਜਾ.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ਅਚਲ AstBasicDType* ਅਚਲ dtypep
                            = VN_CAST(varp->ਬੱਚਾ_DTypep(), BasicDType);
                        ਜੇ (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    ਤਾਰਕਿਕ hier_Sub_ਲੜੀਵਾਰ() ਅਚਲ { ਵਾਪਸ m_hier_Sub_ਲੜੀਵਾਰ; }
    ਤਾਰਕਿਕ is_Hier_ਖੰਡ(ਅਚਲ string& origName) ਅਚਲ {
        ਵਾਪਸ m_hier_ਖੰਡ_Opts_By_Orig_Name.ਲੱਭੋ(origName) != m_hier_ਖੰਡ_Opts_By_Orig_Name.ਸਿਰਾ();
    }
    Ast_ਗੰਢ_Module* ਲੱਭੋ_By_Params(ਅਚਲ string& origName, AstPin* ਪਹਿਲਾ_Pinp,
                                ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        UASSERT(is_Hier_ਖੰਡ(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ਅਚਲ std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_ਖੰਡ_Opts_By_Orig_Name.equal_range(origName);
        ਅਚਲ ਸਵੈਚਾਲਿਤ paramsIt = m_modParams.ਲੱਭੋ(origName);
        UASSERT_OBJ(paramsIt != m_modParams.ਸਿਰਾ(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ਅਚਲ AstPin* typeMismatchPinp = ਖਾਲੀ_ਸੂਚਕ;
        HierMapIt hierIt;
        ਲਈ (hierIt = candidates.ਪਹਿਲਾ; hierIt != candidates.ਦੂਜਾ; ++hierIt) {
            ਤਾਰਕਿਕ ਮਿਲਿਆ = ਸੱਚ;
            ਮਾਪ_t paramIdx = 0;
            ਅਚਲ ParamConstMap& params = m_hierParams[hierIt->ਦੂਜਾ];
            UASSERT(params.ਮਾਪ() == hierIt->ਦੂਜਾ->params().ਮਾਪ(), "not match");
            ਲਈ (AstPin* pinp = ਪਹਿਲਾ_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (!pinp->exprp()) ਜਾਰੀ;
                ਜੇ (ਅਚਲ AstVar* ਅਚਲ modvarp = pinp->modVarp()) {
                    AstConst* ਅਚਲ constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ਅਚਲ ਸਵੈਚਾਲਿਤ paramIt = paramsIt->ਦੂਜਾ.ਲੱਭੋ(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->ਦੂਜਾ.ਸਿਰਾ(), modvarp, "must be registered");
                    ਅਚਲ ਤਾਰਕਿਕ untyped = m_untypedParams.ਗਿਣਤੀ_2(paramIt->ਦੂਜਾ);
                    AstConst* ਅਚਲ defValuep = VN_CAST(paramIt->ਦੂਜਾ->valuep(), Const);
                    ਜੇ (defValuep && areSame(constp, defValuep)) {
                        ਜੇ (!untyped || constp->same_ਮੁੱਲ_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            ਜਾਰੀ;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ਅਚਲ ਸਵੈਚਾਲਿਤ pIt = vlstd::as_const(params).ਲੱਭੋ(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    ਜੇ (pIt == params.ਸਿਰਾ() || paramIdx >= params.ਮਾਪ()
                        || !areSame(constp, pIt->ਦੂਜਾ.get())) {
                        ਮਿਲਿਆ = ਝੂਠ;
                        ਰੋਕੋ;
                    }
                    ਜੇ (untyped && !constp->same_ਮੁੱਲ_Type(pIt->ਦੂਜਾ.get())) {
                        typeMismatchPinp = pinp;
                        ਮਿਲਿਆ = ਝੂਠ;
                        ਰੋਕੋ;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->ਦੂਜਾ.get());
                    ++paramIdx;
                }
            }
            ਜੇ (ਮਿਲਿਆ && paramIdx == hierIt->ਦੂਜਾ->params().ਮਾਪ()) ਰੋਕੋ;
        }
        ਜੇ (hierIt == candidates.ਦੂਜਾ && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ਗੰਢ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        UASSERT_OBJ(hierIt != candidates.ਦੂਜਾ, ਪਹਿਲਾ_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ਅਚਲ Hier_ਖੰਡ_Mod_Map::const_iterator modIt
            = m_hier_ਖੰਡ_Mod.ਲੱਭੋ(hierIt->ਦੂਜਾ->mangledName());
        UASSERT_OBJ(modIt != m_hier_ਖੰਡ_Mod.ਸਿਰਾ(), ਪਹਿਲਾ_Pinp,
                    hierIt->ਦੂਜਾ->mangledName() << " is not found");

        ਅਚਲ ਸਵੈਚਾਲਿਤ it = vlstd::as_const(m_hier_ਖੰਡ_Mod).ਲੱਭੋ(hierIt->ਦੂਜਾ->mangledName());
        ਜੇ (it == m_hier_ਖੰਡ_Mod.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ it->ਦੂਜਾ;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        ਜੇ (pinValuep->isString()) {
            ਵਾਪਸ pinValuep->ਅੰਕ().ਮੰਜ਼ਿਲ_String() == hierOptParamp->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
        }

        ਜੇ (hierOptParamp->isDouble()) {
            ਦੁੱਗਣਾ var;
            ਜੇ (pinValuep->isDouble()) {
                var = pinValuep->ਅੰਕ().ਮੰਜ਼ਿਲ_Double();
            } ਨਹੀਂਤਾਂ {  // Cast from integer to real
                V3_ਸੰਖਿਆ var_ਅੰਕ{pinValuep, V3_ਸੰਖਿਆ::Double{}, 0.0};
                var_ਅੰਕ.opIToRD(pinValuep->ਅੰਕ());
                var = var_ਅੰਕ.ਮੰਜ਼ਿਲ_Double();
            }
            ਵਾਪਸ V3_ਸੰਖਿਆ::epsilonEqual(var, hierOptParamp->ਅੰਕ().ਮੰਜ਼ਿਲ_Double());
        } ਨਹੀਂਤਾਂ {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_ਸੰਖਿਆ var_ਅੰਕ{pinValuep, hierOptParamp->ਅੰਕ().ਚੌੜਾਈ()};
            ਜੇ (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_ਅੰਕ.opRToIRoundS(pinValuep->ਅੰਕ());
            } ਨਹੀਂਤਾਂ ਜੇ (pinValuep->isSigned()) {
                var_ਅੰਕ.opExtendS(pinValuep->ਅੰਕ(), pinValuep->ਅੰਕ().ਚੌੜਾਈ());
            } ਨਹੀਂਤਾਂ {
                var_ਅੰਕ.op_ਸੌਂਪੋ(pinValuep->ਅੰਕ());
            }
            V3_ਸੰਖਿਆ isEq{pinValuep, 1};
            isEq.opEq(var_ਅੰਕ, hierOptParamp->ਅੰਕ());
            ਵਾਪਸ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

ਵਰਗ ParamProcessor final {
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
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    ਵਰਤੋਂ IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    ਵਰਤੋਂ CloneMap = std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, Ast_ਗੰਢ*>;
    ਬਣਤਰ ModInfo final {
        Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        ਸਪਸ਼ਟ ModInfo(Ast_ਗੰਢ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ਅਚਲ std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ਅਚਲ std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::ਸੈੱਟ<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_ਸੈੱਟ m_all_Module_ਨਾਂ;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ਅਚਲ std::string, std::string> m_ਮੁੱਲ_ਨਾਂ;  // Parameter value text to its name
    std::ਸੈੱਟ<std::string> m_ਮੁੱਲ_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ਸਥਿਰ constexpr std::string::ਮਾਪ_type DIGEST_DIGITS = 8;

    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_ਖੰਡਾਂ m_hier_ਖੰਡਾਂ;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    ਵਰਤੋਂ Default_ਮੁੱਲ_Map = std::map<std::string, Ast_ਗੰਢ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ਗੰਢ_Module*, Default_ਮੁੱਲ_Map> m_default_Parameter_ਮੁੱਲਾਂ;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ਅਚਲ Ast_ਗੰਢ_Module*, ਅਚਲ AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::ਸਦਿਸ਼<std::pair<AstParamTypeDType*, ਪੂਰਨ>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, ਪੂਰਨ> m_param_ਸੂਚਕਾਂਕ;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_ਸੈੱਟ<ਅਚਲ AstClass*> m_default_Clone_ਵਿੱਚ_ਤਰੱਕੀ;

    std::ਸਦਿਸ਼<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ਸਥਿਰ ਖਾਲੀ make_Small_ਨਾਂ(Ast_ਗੰਢ_Module* modp) {
        std::ਸਦਿਸ਼<ਪੂਰਨ> ਵਰਤਿਆ_Letter;
        ਵਰਤਿਆ_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isGParam() || varp->isIfaceRef()) {
                    ਅੱਖਰ ch = varp->name()[0];
                    ch = std::ਵੱਡਾ_ਅੱਖਰ(ch);
                    ਜੇ (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(ਵਰਤਿਆ_Letter[static_cast<ਪੂਰਨ>(ch)] * 256 + ch);
                    ਵਰਤਿਆ_Letter[static_cast<ਪੂਰਨ>(ch)]++;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ typep = VN_CAST(stmtp, ParamTypeDType)) {
                ਅਚਲ ਅੱਖਰ ch = 'T';
                typep->user3(ਵਰਤਿਆ_Letter[static_cast<ਪੂਰਨ>(ch)] * 256 + ch);
                ਵਰਤਿਆ_Letter[static_cast<ਪੂਰਨ>(ch)]++;
            }
        }
    }
    ਸਥਿਰ string paramSmallName(Ast_ਗੰਢ_Module* modp, Ast_ਗੰਢ* varp) {
        ਜੇ (varp->user3() <= 1) make_Small_ਨਾਂ(modp);
        ਪੂਰਨ ਸੂਚਕਾਂਕ = varp->user3() / 256;
        ਅਚਲ ਅੱਖਰ ch = varp->user3() & 255;
        string st = cvt_ਮੰਜ਼ਿਲ_Str(ch);
        ਜਦੋਂ (ਸੂਚਕਾਂਕ) {
            st += cvt_ਮੰਜ਼ਿਲ_Str(static_cast<ਅੱਖਰ>((ਸੂਚਕਾਂਕ % 25) + 'A'));
            ਸੂਚਕਾਂਕ /= 26;
        }
        ਵਾਪਸ st;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_Aggregate_Param_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ਸਥਿਰ string param_ਮੁੱਲ_String(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (ਅਚਲ AstRefDType* ਅਚਲ refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp();
        }
        string ਕੁੰਜੀ = nodep->name();
        ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            ਜੇ (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ਕੁੰਜੀ = ifrtp->cellp()->modp()->name();
            } ਨਹੀਂਤਾਂ ਜੇ (ifrtp->ifacep()) {
                ਕੁੰਜੀ = ifrtp->ifacep()->name();
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtypep
                   = VN_CAST(nodep, ਗੰਢ_UOr_Struct_DType)) {
            ਕੁੰਜੀ += " ";
            ਕੁੰਜੀ += dtypep->verilogKwd();
            ਕੁੰਜੀ += " {";
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ਕੁੰਜੀ += param_ਮੁੱਲ_String(memberp);
                ਕੁੰਜੀ += ";";
            }
            ਕੁੰਜੀ += "}";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberDType* ਅਚਲ dtypep = VN_CAST(nodep, MemberDType)) {
            ਕੁੰਜੀ += " ";
            ਕੁੰਜੀ += param_ਮੁੱਲ_String(dtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ dtypep = VN_CAST(nodep, BasicDType)) {
            ਜੇ (dtypep->isSigned()) ਕੁੰਜੀ += " signed";
            ਜੇ (dtypep->isRanged()) {
                ਕੁੰਜੀ += "[" + cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਖੱਬੇ()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਸੱਜੇ()) + "]";
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ dtypep = VN_CAST(nodep, PackArrayDType)) {
            ਕੁੰਜੀ += "[";
            ਕੁੰਜੀ += cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਖੱਬੇ());
            ਕੁੰਜੀ += ":";
            ਕੁੰਜੀ += cvt_ਮੰਜ਼ਿਲ_Str(dtypep->ਸੱਜੇ());
            ਕੁੰਜੀ += "] ";
            ਕੁੰਜੀ += param_ਮੁੱਲ_String(dtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਨਮੂਨਾ* ਅਚਲ patp = VN_CAST(nodep, ਨਮੂਨਾ)) {
            // A parameter override pattern, keyed before the specialized module types it
            ਕੁੰਜੀ += "'{";
            ਲਈ (ਅਚਲ AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                ਜੇ (memp->isDefault()) ਕੁੰਜੀ += "default";
                ਜੇ (ਅਚਲ Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(memp->keyp(), ਪਾਠ_2)) {
                    ਕੁੰਜੀ += textp->ਪਾਠ();
                } ਨਹੀਂਤਾਂ ਜੇ (memp->keyp()) {
                    ਕੁੰਜੀ += param_ਮੁੱਲ_String(memp->keyp());
                }
                ਕੁੰਜੀ += ":";
                ਜੇ (memp->repp()) ਕੁੰਜੀ += param_ਮੁੱਲ_String(memp->repp()) + "x";
                ਲਈ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                    ਕੁੰਜੀ += param_ਮੁੱਲ_String(valuep) + ";";
                }
                ਕੁੰਜੀ += ",";
            }
            ਕੁੰਜੀ += "}";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ਕੁੰਜੀ += "{";
            ਲਈ (ਅਚਲ Ast_Cons_Array_ਵਸਤੂ* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_ਵਸਤੂ)) {
                ਕੁੰਜੀ += cvt_ਮੰਜ਼ਿਲ_Str(itemp->ਸੂਚਕਾਂਕ()) + ":" + param_ਮੁੱਲ_String(itemp->valuep()) + ",";
            }
            ਜੇ (consArrayp->defaultp()) {
                ਕੁੰਜੀ += "default:" + param_ਮੁੱਲ_String(consArrayp->defaultp()) + ",";
            }
            ਕੁੰਜੀ += "}";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConsStruct* ਅਚਲ consp = VN_CAST(nodep, ConsStruct)) {
            ਕੁੰਜੀ += "{";
            ਲਈ (ਅਚਲ Ast_Cons_Struct_ਵਸਤੂ* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_ਵਸਤੂ)) {
                ਕੁੰਜੀ += param_ਮੁੱਲ_String(itemp->valuep());
                ਕੁੰਜੀ += ",";
            }
            ਕੁੰਜੀ += "}";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            ਜੇ (classRefp->classp()) {
                ਅਚਲ string& className = classRefp->classp()->name();
                ਅਚਲ string& origName = classRefp->classp()->origName();
                ਅਚਲ ਤਾਰਕਿਕ isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                ਜੇ (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } ਨਹੀਂਤਾਂ ਜੇ (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ਕੁੰਜੀ = className;
                } ਨਹੀਂਤਾਂ {
                    // Unspecialized class with no params - use origName
                    ਕੁੰਜੀ = origName;
                }
            } ਨਹੀਂਤਾਂ {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ਕੁੰਜੀ = paramElemString(dtypep->subDTypep()) + "$" + cvt_ਮੰਜ਼ਿਲ_Str(dtypep->declRange());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ dtypep = VN_CAST(nodep, ਕਤਾਰ_DType)) {
            ਕੁੰਜੀ = paramElemString(dtypep->subDTypep()) + "$[$";
            ਜੇ (dtypep->boundConst()) ਕੁੰਜੀ += ":" + cvt_ਮੰਜ਼ਿਲ_Str(dtypep->boundConst());
            ਕੁੰਜੀ += "]";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ dtypep = VN_CAST(nodep, DynArrayDType)) {
            ਕੁੰਜੀ = paramElemString(dtypep->subDTypep()) + "$[]";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAssocArrayDType* ਅਚਲ dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ਕੁੰਜੀ = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ਕੁੰਜੀ_DTypep()) + "]";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstWildcardArrayDType* ਅਚਲ dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ਕੁੰਜੀ = paramElemString(dtypep->subDTypep()) + "$[*]";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstEnumDType* ਅਚਲ dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ਕੁੰਜੀ += " enum {";
            ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(itemp->valuep(), Const);
                ਕੁੰਜੀ += itemp->name() + "=" + (constp ? constp->ਅੰਕ().ascii() : "?") + ";";
            }
            ਕੁੰਜੀ += "}";
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = VN_CAST(nodep, ਗੰਢ_DType)) {
            ਕੁੰਜੀ += dtypep->prettyDTypeName(ਸੱਚ);
        }
        UASSERT_OBJ(!ਕੁੰਜੀ.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Parameter yielded no value string");
        ਵਾਪਸ ਕੁੰਜੀ;
    }
    // As paramValueString(), for the element or key type of a container
    ਸਥਿਰ string paramElemString(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ਅਚਲ AstClassRefDType* ਅਚਲ classRefp
            = VN_CAST(dtypep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp(), ClassRefDType);
        ਜੇ (classRefp && classRefp->paramsp()) ਵਾਪਸ classRefp->prettyDTypeName(ਸੱਚ);
        ਵਾਪਸ param_ਮੁੱਲ_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ਸਥਿਰ string digestSuffix(ਅਚਲ string& ਪਾਠ, std::ਸੈੱਟ<string>& usedr) {
        ਅਚਲ string hex = VHashSha512{ਪਾਠ}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::ਮਾਪ_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        ਜਦੋਂ (digits < hex.ਮਾਪ() && !usedr.ਪਾਓ(hex.substr(0, digits)).ਦੂਜਾ) ++digits;
        ਵਾਪਸ hex.substr(0, digits);
    }
    string param_ਮੁੱਲ_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        ਜੇ (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ਅਚਲ string ਪਾਠ = param_ਮੁੱਲ_String(nodep);
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ਮੁੱਲ_ਨਾਂ.emplace(ਪਾਠ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ = "z" + digestSuffix(ਪਾਠ, m_ਮੁੱਲ_Suffixes);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }
    string moduleCalcName(ਅਚਲ Ast_ਗੰਢ_Module* srcModp, ਅਚਲ string& longname) {
        string newname = longname;
        ਜੇ (longname.ਲੰਮਾਈ() > 30) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_longMap.emplace(longname, "");
            ਜੇ (pair.ਦੂਜਾ) {
                // We use all upper case above, so lower here can't conflict
                pair.ਪਹਿਲਾ->ਦੂਜਾ
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        ਵਾਪਸ newname;
    }
    ਸਥਿਰ Ast_ਗੰਢ_DType* arraySubDTypep(Ast_ਗੰਢ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            ਵਾਪਸ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        ਜੇ (ਅਚਲ AstBracketArrayDType* ਅਚਲ adtypep = VN_CAST(nodep, BracketArrayDType)) {
            ਵਾਪਸ adtypep->subDTypep();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ਸਥਿਰ Ast_ਗੰਢ_DType* arraySubDTypeDeepp(Ast_ਗੰਢ_DType* nodep) {
        ਜਦੋਂ (Ast_ਗੰਢ_DType* ਅਚਲ subp = arraySubDTypep(nodep)) nodep = subp;
        ਵਾਪਸ nodep;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isString(Ast_ਗੰਢ_DType* nodep) {
        ਜੇ (AstBasicDType* ਅਚਲ basicp = VN_CAST(nodep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp(), BasicDType))
            ਵਾਪਸ basicp->isString();
        ਵਾਪਸ ਝੂਠ;
    }
    ਖਾਲੀ collectPins(CloneMap* clonemapp, Ast_ਗੰਢ_Module* modp, ਤਾਰਕਿਕ originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਅਚਲ Ast_ਗੰਢ* originalParamp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ਜੇ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ਜੇ (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            ਜੇ (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    ਖਾਲੀ relinkPins(ਅਚਲ CloneMap* clonemapp, AstPin* startpinp) {
        ਲਈ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ਅਚਲ ਸਵੈਚਾਲਿਤ cloneiter = clonemapp->ਲੱਭੋ(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->ਸਿਰਾ(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->ਦੂਜਾ, Var));
            } ਨਹੀਂਤਾਂ ਜੇ (pinp->modPTypep()) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ cloneiter = clonemapp->ਲੱਭੋ(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->ਸਿਰਾ(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->ਦੂਜਾ, ParamTypeDType));
            } ਨਹੀਂਤਾਂ {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    ਖਾਲੀ relinkPinsByName(AstPin* startpinp, Ast_ਗੰਢ_Module* modp) {
        std::map<ਅਚਲ string, AstVar*> name_ਮੰਜ਼ਿਲ_Pin;
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_ਮੰਜ਼ਿਲ_Pin.emplace(varp->name(), varp);
                }
            }
        }
        ਲਈ (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = pinp->modVarp()) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ varIt = vlstd::as_const(name_ਮੰਜ਼ਿਲ_Pin).ਲੱਭੋ(varp->name());
                UASSERT_OBJ(varIt != name_ਮੰਜ਼ਿਲ_Pin.ਸਿਰਾ(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->ਦੂਜਾ);
            }
        }
    }
    ਸਥਿਰ ਤਾਰਕਿਕ hasDescendantDefparams(ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                       std::ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ_Module*>& ਦੇਖਿਆ) {
        ਜੇ (!ਦੇਖਿਆ.ਪਾਓ(modp).ਦੂਜਾ) ਵਾਪਸ ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ਅਚਲ cellp = VN_CAST(stmtp, Cell);
            ਜੇ (!cellp) ਜਾਰੀ;
            ਲਈ (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (!pinp->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਦੇਖਿਆ.erase(modp);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
            ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ ਬੱਚਾ_Modp = cellp->modp()) {
                ਜੇ (hasDescendantDefparams(ਬੱਚਾ_Modp, ਦੇਖਿਆ)) {
                    ਦੇਖਿਆ.erase(modp);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }
        ਦੇਖਿਆ.erase(modp);
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ hasDescendantDefparams(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        std::ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ_Module*> ਦੇਖਿਆ;
        ਵਾਪਸ hasDescendantDefparams(modp, ਦੇਖਿਆ);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ਅਚਲ AstPin* nestedDefparamp(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_nestedDefparamps.emplace(modp, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (!pair.ਦੂਜਾ) ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;  // Searched, or being searched
        ਅਚਲ AstPin* foundp = ਖਾਲੀ_ਸੂਚਕ;
        modp->exists([&](ਅਚਲ AstCell* cellp) {
            ਲਈ (ਅਚਲ AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (!pinp->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ()) foundp = pinp;
            }
            ਜੇ (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            ਵਾਪਸ foundp != ਖਾਲੀ_ਸੂਚਕ;
        });
        pair.ਪਹਿਲਾ->ਦੂਜਾ = foundp;
        ਵਾਪਸ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    ਖਾਲੀ checkSupportedParam(Ast_ਗੰਢ_Module* modp, AstPin* pinp) ਅਚਲ {
        // ConsArray is not supported because that can not be set via -G
        // option.
        ਜੇ (pinp->modVarp()) {
            ਤਾਰਕਿਕ supported = ਝੂਠ;
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(pinp->exprp(), Const)) {
                supported = ਸੱਚ;
                ਜੇ (constp->ਅੰਕ().isString()
                    && !V3_Hier_ਖੰਡ::stringParamPassable(constp->ਅੰਕ().ਮੰਜ਼ਿਲ_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            ਜੇ (!supported) {
                pinp->v3error(
                    Ast_ਗੰਢ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstParamTypeDType* ਅਚਲ typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ਗੰਢ::prettyNameQ(modp->origName()));
        }
    }
    ਤਾਰਕਿਕ moduleExists(ਅਚਲ string& modName) ਅਚਲ {
        ਜੇ (m_all_Module_ਨਾਂ.ਲੱਭੋ(modName) != m_all_Module_ਨਾਂ.ਸਿਰਾ()) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_modNameMap.ਲੱਭੋ(modName) != m_modNameMap.ਸਿਰਾ()) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    }

    string parameterized_Hier_ਖੰਡ_Name(Ast_ਗੰਢ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_ਖੰਡ(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ਗੰਢ*> pins;

        AstPin* pinp = paramPinsp;
        ਜਦੋਂ (pinp) {
            checkSupportedParam(modp, pinp);
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = pinp->modVarp()) {
                ਜੇ (!pinp->exprp()) ਜਾਰੀ;
                ਜੇ (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ਗੰਢ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_default_Parameter_ਮੁੱਲਾਂ.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        ਜੇ (pair.ਦੂਜਾ) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            ਜੇ (ਅਚਲ AstPin* ਅਚਲ defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ਗੰਢ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_ਮੁੱਲ_Map params;
            ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                    ਜੇ (varp->isGParam()) {
                        AstConst* ਅਚਲ constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ਗੰਢ* ਅਚਲ dtypep = static_cast<Ast_ਗੰਢ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.ਪਹਿਲਾ->ਦੂਜਾ = std::move(params);
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ paramsIt = pair.ਪਹਿਲਾ;
        ਜੇ (paramsIt->ਦੂਜਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        ਲਈ (ਸਵੈਚਾਲਿਤ&& default_ਮੁੱਲ : paramsIt->ਦੂਜਾ) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ pinIt = pins.ਲੱਭੋ(default_ਮੁੱਲ.ਪਹਿਲਾ);
            // If the pin does not have a value assigned, use the default one.
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep = pinIt == pins.ਸਿਰਾ() ? default_ਮੁੱਲ.ਦੂਜਾ : pinIt->ਦੂਜਾ;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_ਮੁੱਲ.ਪਹਿਲਾ + "=";
            // constp can be nullptr

            ਜੇ (ਅਚਲ AstConst* ਅਚਲ p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->ਅੰਕ().ascii(ਝੂਠ);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_ਰੁੱਖ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_longMap.ਲੱਭੋ(longname);
        ਜੇ (iter != m_longMap.ਸਿਰਾ()) ਵਾਪਸ iter->ਦੂਜਾ;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.ਪਾਓ(longname);
        ਜਦੋਂ (ਸੱਚ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ਅਚਲ string hashStr = hashStrGen.digestHex();
            ਲਈ (string::ਮਾਪ_type i = 1; i < hashStr.ਮਾਪ(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                ਜੇ (newName.at(newName.ਮਾਪ() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                ਜੇ (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    ਵਾਪਸ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.ਪਾਓ(V3Os::trueRandom(64));
        }
    }
    ਖਾਲੀ replaceRefsRecurse(Ast_ਗੰਢ* ਅਚਲ nodep, ਅਚਲ AstClass* ਅਚਲ oldClassp,
                            AstClass* ਅਚਲ newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        ਜੇ (AstClassRefDType* ਅਚਲ classRefp = VN_CAST(nodep, ClassRefDType)) {
            ਜੇ (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassOrPackageRef* ਅਚਲ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ਜੇ (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstTypedef* ਅਚਲ typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            ਜੇ (typedefp->subDTypep()) {
                ਜੇ (AstClassRefDType* ਅਚਲ classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    ਜੇ (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskRefp = VN_CAST(nodep, ਗੰਢ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            ਜੇ (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ oldTaskp = ftaskRefp->taskp()) {
                ਜੇ (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        ਜੇ (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        ਜੇ (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        ਜੇ (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        ਜੇ (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        ਜੇ (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    ਵਰਗ VarXRefRelinkVisitor final : ਜਨਤਕ VNVisitor {
        Ast_ਗੰਢ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ਗੰਢ_Module*> m_varModuleMap;  // Cache var->module lookups

    ਜਨਤਕ:
        ਸਪਸ਼ਟ VarXRefRelinkVisitor(Ast_ਗੰਢ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    ਨਿਜੀ:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ਗੰਢ_Module* ਲੱਭੋ_Var_Module(AstVar* varp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_varModuleMap.ਲੱਭੋ(varp);
            ਜੇ (it != m_varModuleMap.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
            Ast_ਗੰਢ_Module* varModp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ* np = varp; np; np = np->backp()) {
                ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(np, ਗੰਢ_Module)) {
                    varModp = modp;
                    ਰੋਕੋ;
                }
            }
            m_varModuleMap[varp] = varModp;
            ਵਾਪਸ varModp;
        }

        ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
            AstVar* ਅਚਲ varp = nodep->varp();
            ਜੇ (!varp) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ਅਚਲ string& dotted = nodep->dotted();
            ਜੇ (dotted.ਖਾਲੀ_ਥਾਵਾਂ()) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            ਅਚਲ ਮਾਪ_t dotPos = dotted.ਲੱਭੋ('.');
            ਅਚਲ string ਪੋਰਟ_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            ਜੇ (ਪੋਰਟ_Name.ਖਾਲੀ_ਥਾਵਾਂ()) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            // Find the interface port variable in the cloned module
            AstVar* ਪੋਰਟ_Varp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (AstVar* ਅਚਲ varChkp = VN_CAST(stmtp, Var)) {
                    ਜੇ (varChkp->name() == ਪੋਰਟ_Name && varChkp->isIfaceRef()) {
                        ਪੋਰਟ_Varp = varChkp;
                        ਰੋਕੋ;
                    }
                }
            }
            ਜੇ (!ਪੋਰਟ_Varp) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ਅਚਲ irefp = VN_CAST(ਪੋਰਟ_Varp->subDTypep(), IfaceRefDType);
            ਜੇ (!irefp) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            Ast_ਗੰਢ_Module* ਅਚਲ newIfacep = irefp->iface_ਰਾਹੀਂ_Cellp();
            ਜੇ (!newIfacep) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ਗੰਢ_Module* ਅਚਲ varModp = ਲੱਭੋ_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            ਜੇ (varModp && varModp != newIfacep) {
                ਲਈ (Ast_ਗੰਢ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (AstVar* ਅਚਲ newVarp = VN_CAST(stmtp, Var)) {
                        ਜੇ (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            ਰੋਕੋ;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    ਖਾਲੀ fixupCrossModuleRefDTypes(Ast_ਗੰਢ_Module* newModp) {
        ਜੇ (!V3LinkDotIfaceCapture::enabled()) ਵਾਪਸ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        ਪੂਰਨ fixed_ਗਿਣਤੀ = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = V3LinkDotIfaceCapture::capture_ਟੈਗ(refp);
            ਜੇ (!tagp) ਵਾਪਸ;

            Ast_ਗੰਢ_Module* ਅਚਲ correctModp
                = V3LinkDotIfaceCapture::follow_Cell_ਰਸਤਾ(newModp, tagp->m_cell_ਰਸਤਾ);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_ਰਸਤਾ << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            ਜੇ (!correctModp || correctModp->dead()) ਵਾਪਸ;
            ਜੇ (correctModp->parameterizedTemplate()) ਵਾਪਸ;

            ਤਾਰਕਿਕ fixed = ਝੂਠ;
            ਜੇ (refp->typedefp()) {
                ਜੇ (AstTypedef* ਅਚਲ newTdp = V3LinkDotIfaceCapture::ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    ਜੇ (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = ਸੱਚ;
                }
            }
            ਜੇ (!fixed && refp->refDTypep()) {
                ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ newDtp = V3LinkDotIfaceCapture::ਲੱਭੋ_DType_ਵਿੱਚ_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = ਸੱਚ;
                }
            }
            ਜੇ (fixed) ++fixed_ਗਿਣਤੀ;
        });
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("IfaceCapture, Ledger fixups in V3Param", fixed_ਗਿਣਤੀ);
    }

    // Return true on success, false on error
    ਤਾਰਕਿਕ deepCloneModule(Ast_ਗੰਢ_Module* srcModp, Ast_ਗੰਢ* ifErrorp, AstPin* paramsp,
                         ਅਚਲ string& newname, ਅਚਲ IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ਗੰਢ_Module* newModp;
        ਜੇ (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_ਰੁੱਖ(ਝੂਠ), ਗੰਢ_Module);
        } ਨਹੀਂਤਾਂ {
            newModp = srcModp->clone_ਰੁੱਖ(ਝੂਠ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        ਜੇ (AstIface* ਅਚਲ newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(ਸੱਚ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(ਝੂਠ);

        newModp->name(newname);
        newModp->user2(ਝੂਠ);  // We need to re-recurse this module once changed
        newModp->recursive(ਝੂਠ);
        newModp->recursiveClone(ਝੂਠ);
        newModp->hasGParam(ਝੂਠ);
        // Recursion may need level cleanups
        ਜੇ (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        ਜੇ ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_ਡੂੰਘਾਈ() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_ਡੂੰਘਾਈ());
            VL_DO_DANGLING(newModp->delete_ਰੁੱਖ(), newModp);
            ਵਾਪਸ ਝੂਠ;
        }

        ਜੇ (AstClass* ਅਚਲ newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ਗੰਢ* insertp = srcModp;
        ਜਦੋਂ (VN_IS(insertp->nextp(), ਗੰਢ_Module)
               && VN_AS(insertp->nextp(), ਗੰਢ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_modNameMap.ਲੱਭੋ(newname);
        CloneMap* ਅਚਲ clonemapp = &(iter->ਦੂਜਾ.m_cloneMap);
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
        ਤਾਰਕਿਕ needRelinkByName = ਝੂਠ;
        ਜੇ (paramsp) {
            ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (pinp->modVarp() && clonemapp->ਲੱਭੋ(pinp->modVarp()) == clonemapp->ਸਿਰਾ()) {
                    needRelinkByName = ਸੱਚ;
                    ਰੋਕੋ;
                }
            }
        }
        ਜੇ (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } ਨਹੀਂਤਾਂ {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        ਲਈ (ਸਵੈਚਾਲਿਤ it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ਅਚਲ AstIfaceRefDType* ਅਚਲ ਪੋਰਟ_Irefp = it->ਪਹਿਲਾ;
            ਅਚਲ AstIfaceRefDType* ਅਚਲ pinIrefp = it->ਦੂਜਾ;
            AstIfaceRefDType* ਅਚਲ cloneIrefp = ਪੋਰਟ_Irefp->clonep();
            UINFO(8, "     IfaceOld " << ਪੋਰਟ_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, ਪੋਰਟ_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_ਰਾਹੀਂ_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        ਜੇ (!ifaceRefRefs.ਖਾਲੀ_ਥਾਵਾਂ()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (pinp->exprp()) {
                ਜੇ (AstVar* ਅਚਲ modvarp = pinp->modVarp()) {
                    Ast_ਗੰਢ* ਅਚਲ newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ਅਚਲ exprp = VN_CAST(newp, Const);
                    AstConst* ਅਚਲ origp = VN_CAST(modvarp->valuep(), Const);
                    ਅਚਲ ਤਾਰਕਿਕ overridden
                        = !(origp && exprp && Parameterized_Hier_ਖੰਡਾਂ::areSame(exprp, origp));
                    // Remove any existing parameter
                    ਜੇ (modvarp->valuep()) modvarp->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_ਰੁੱਖ(ਝੂਠ));
                    modvarp->overriddenParam(overridden);
                } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ modptp = pinp->modPTypep()) {
                    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = VN_AS(pinp->exprp(), ਗੰਢ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    ਜੇ (modptp->ਬੱਚਾ_DTypep()) modptp->ਬੱਚਾ_DTypep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                    // Set this parameter to value requested by cell
                    modptp->ਬੱਚਾ_DTypep(dtypep->clone_ਰੁੱਖ(ਝੂਠ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        ਵਾਪਸ ਸੱਚ;
    }
    ਅਚਲ ModInfo* module_ਲੱਭੋ_Or_Clone(Ast_ਗੰਢ_Module* srcModp, Ast_ਗੰਢ* ifErrorp, AstPin* paramsp,
                                     ਅਚਲ string& newname, ਅਚਲ IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        ਸਵੈਚਾਲਿਤ it = m_modNameMap.ਲੱਭੋ(newname);
        ਜੇ (it != m_modNameMap.ਸਿਰਾ()) {
            UINFO(4, "     De-parameterize to prev: " << it->ਦੂਜਾ.m_modp);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            it = m_modNameMap.ਲੱਭੋ(newname);
            UASSERT(it != m_modNameMap.ਸਿਰਾ(), "should find just-made module");
        }
        ਅਚਲ ModInfo* ਅਚਲ modInfop = &(it->ਦੂਜਾ);
        ਵਾਪਸ modInfop;
    }

    ਖਾਲੀ convert_ਮੰਜ਼ਿਲ_Stringp(Ast_ਗੰਢ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        ਜੇ (constp && !constp->ਅੰਕ().isString()) {
            constp->replaceWith(
                ਨਵਾਂ AstConst{constp->fileline(), AstConst::String{}, constp->ਅੰਕ().ਮੰਜ਼ਿਲ_String()});
            VL_DO_DANGLING(constp->delete_ਰੁੱਖ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    ਖਾਲੀ constify_Member_ਮੁੱਲ(AstVar* varp, std::ਸੈੱਟ<AstVar*>& ਵਿੱਚ_ਤਰੱਕੀ) {
        ਜੇ (!varp->valuep() || VN_IS(varp->valuep(), Const)) ਵਾਪਸ;
        ਜੇ (!ਵਿੱਚ_ਤਰੱਕੀ.ਪਾਓ(varp).ਦੂਜਾ) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            varp->valuep(ਨਵਾਂ AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            ਵਾਪਸ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::ਸੈੱਟ<AstVar*> siblings;
        varp->valuep()->foreach([&](ਅਚਲ AstVarRef* refp) {
            AstVar* ਅਚਲ refVarp = refp->varp();
            ਜੇ (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.ਪਾਓ(refVarp);
            }
        });
        ਲਈ (AstVar* ਅਚਲ sibp : siblings) constify_Member_ਮੁੱਲ(sibp, ਵਿੱਚ_ਤਰੱਕੀ);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        ਵਿੱਚ_ਤਰੱਕੀ.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ਸਥਿਰ AstParseRef* memberParseRef(Ast_ਗੰਢ* nodep) {
        ਜੇ (AstParseRef* ਅਚਲ refp = VN_CAST(nodep, ParseRef)) ਵਾਪਸ refp;
        Ast_ਗੰਢ_Pre_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Pre_Sel);
        ਵਾਪਸ selp ? VN_CAST(selp->fromp(), ParseRef) : ਖਾਲੀ_ਸੂਚਕ;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ਸਥਿਰ ਖਾਲੀ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_Pre_Sel* ਅਚਲ selp = VN_AS(dotp->rhsp(), ਗੰਢ_Pre_Sel);
            ਜੇ (Ast_Attr_ਦਾ* ਅਚਲ attrp = selp->attrp()) {
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_ਰੁੱਖ(ਝੂਠ));
                    VL_DO_DANGLING(oldFromp->delete_ਰੁੱਖ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_ਰੁੱਖ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_ਪਿੱਠ());
        }
        VL_DO_DANGLING(dotp->delete_ਰੁੱਖ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ਸਥਿਰ ਤਾਰਕਿਕ is_Deferred_Member_ਅਧਾਰ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (VN_IS(nodep, MemberSel)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Pre_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Pre_Sel)) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDTypep = selp->fromp()->dtypep();
            ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ਗੰਢ_Array_DType) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (arrayDTypep) {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = arrayDTypep->subDTypep();
                ਵਾਪਸ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ਗੰਢ_UOr_Struct_DType);
            }
            ਵਾਪਸ is_Deferred_Member_ਅਧਾਰ(selp->fromp());
        }
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->dtypep();
        ਵਾਪਸ dtypep && VN_IS(dtypep->skipRefOrNullp(), ਗੰਢ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ਸਥਿਰ ਖਾਲੀ resolveMemberDot(AstDot* dotp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = VN_CAST(dotp->lhsp(), ਗੰਢ_ਵਿਅੰਜਕ);
        AstParseRef* ਅਚਲ memberRefp = memberParseRef(dotp->rhsp());
        ਜੇ (!lhsp || !memberRefp || !is_Deferred_Member_ਅਧਾਰ(lhsp)) ਵਾਪਸ;
        AstMemberSel* ਅਚਲ newp = ਨਵਾਂ AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_ਪਿੱਠ(),
                                                    VFlag_ਬੱਚਾ_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    ਖਾਲੀ resolve_Dot_ਮੰਜ਼ਿਲ_Typedef(Ast_ਗੰਢ* exprp) {
        AstDot* ਅਚਲ dotp = VN_CAST(exprp, Dot);
        ਜੇ (!dotp) ਵਾਪਸ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ਅਚਲ classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        ਜੇ (!classRefp) {
            resolveMemberDot(dotp);
            ਵਾਪਸ;
        }
        AstParseRef* ਅਚਲ parseRefp = memberParseRef(dotp->rhsp());
        ਜੇ (!parseRefp) ਵਾਪਸ;

        ਅਚਲ AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ਅਚਲ paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        ਜੇ (paramTypep) {
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = paramTypep->subDTypep();
            AstClassRefDType* ਅਚਲ classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (classRefDTypep) {
                AstClass* ਅਚਲ srcClassp = classRefDTypep->classp();
                ਜੇ (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    ਜੇ (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        ਜੇ (classRefp->paramsp()) {
            AstClass* ਅਚਲ srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            ਜੇ (srcClassp && srcClassp->hasGParam()) {
                ਜੇ (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        ਜੇ (AstClassRefDType* ਅਚਲ crp = class_Ref_DType_ਦਾ_ਗੰਢ(classRefp->classOrPackageNodep())) {
            ਜੇ (AstClass* ਅਚਲ srcClassp = crp->classp()) {
                ਜੇ (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    ਜੇ (AstClass* ਅਚਲ newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        ਜੇ (!lhsClassp) ਵਾਪਸ;

        Ast_ਗੰਢ* ਅਚਲ memberp = m_memberMap.ਲੱਭੋ_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        ਜੇ (AstTypedef* ਅਚਲ tdefp = VN_CAST(memberp, Typedef)) {
            ਜੇ (parseRefp != dotp->rhsp()) ਵਾਪਸ;
            AstRefDType* ਅਚਲ refp = ਨਵਾਂ AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_ਰੁੱਖ(), dotp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    ਖਾਲੀ substituteParamMember(AstDot* ਅਚਲ dotp, AstParseRef* ਅਚਲ memberRefp,
                               AstVar* ਅਚਲ varp) {
        ਜੇ (!varp->isParam() || !varp->valuep()) ਵਾਪਸ;
        ਜੇ (!VN_IS(varp->valuep(), Const)) {
            std::ਸੈੱਟ<AstVar*> ਵਿੱਚ_ਤਰੱਕੀ;
            constify_Member_ਮੁੱਲ(varp, ਵਿੱਚ_ਤਰੱਕੀ);
        }
        AstConst* ਅਚਲ constp = VN_CAST(varp->valuep(), Const);
        ਜੇ (!constp) ਵਾਪਸ;
        AstConst* ਅਚਲ newp = constp->clone_ਰੁੱਖ(ਝੂਠ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    ਖਾਲੀ resolveParamClassRefDType(AstRefDType* refp) {
        ਜੇ (refp->typedefp() || refp->refDTypep()) ਵਾਪਸ;

        AstClassOrPackageRef* ਅਚਲ classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        ਜੇ (!classRefp) ਵਾਪਸ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        ਜੇ (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            ਵਾਪਸ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ਗੰਢ* targetp = classRefp->classOrPackageNodep();
        ਜਦੋਂ (ਅਚਲ AstTypedef* ਅਚਲ tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtargetp = VN_CAST(targetp, ਗੰਢ_DType))
            targetp = dtargetp->skipRefOrNullp();
        ਜੇ (!targetp) ਵਾਪਸ;
        AstClassRefDType* ਅਚਲ classRefDTypep = VN_CAST(targetp, ClassRefDType);
        ਜੇ (!classRefDTypep) ਵਾਪਸ;
        srcClassp = classRefDTypep->classp();
        ਜੇ (!srcClassp) ਵਾਪਸ;

        AstClass* newClassp = srcClassp;
        ਜੇ (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ਅਚਲ typedefp
            = VN_CAST(m_memberMap.ਲੱਭੋ_Member(newClassp, refp->name()), Typedef);
        ਜੇ (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ਸਥਿਰ ਤਾਰਕਿਕ defaultParamsHaveTypeQueryOnParamType(ਅਚਲ AstClassRefDType* nodep) {
        ਤਾਰਕਿਕ ਮਿਲਿਆ = ਝੂਠ;
        nodep->foreach([&](ਅਚਲ Ast_Attr_ਦਾ* attrp) {
            ਜੇ (ਮਿਲਿਆ || !attrp->attrType().isTypeQuery()) ਵਾਪਸ;
            ਅਚਲ AstRefDType* ਅਚਲ refp = VN_CAST(attrp->fromp(), RefDType);
            ਜੇ (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) ਮਿਲਿਆ = ਸੱਚ;
        });
        ਵਾਪਸ ਮਿਲਿਆ;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    ਤਾਰਕਿਕ classTypeMatchesDefaultClone(ਅਚਲ Ast_ਗੰਢ_DType* exprp, ਅਚਲ Ast_ਗੰਢ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ ਵਿਅੰਜਕ_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ origClassRefp = VN_CAST(origp, ClassRefDType);
        ਜੇ (!ਵਿਅੰਜਕ_Class_Refp || !origClassRefp) ਵਾਪਸ ਝੂਠ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        ਜੇ (defaultClonep && defaultClonep == ਵਿਅੰਜਕ_Class_Refp->classp()) ਵਾਪਸ ਸੱਚ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        ਜੇ (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) ਵਾਪਸ ਝੂਠ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        ਜੇ (!origClassRefp->classp()->hasGParam()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (origClassRefp->classp()->origName() != ਵਿਅੰਜਕ_Class_Refp->classp()->origName())
            ਵਾਪਸ ਝੂਠ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        ਜੇ (!m_default_Clone_ਵਿੱਚ_ਤਰੱਕੀ.ਪਾਓ(origClassRefp->classp()).ਦੂਜਾ) ਵਾਪਸ ਝੂਠ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ਅਚਲ origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_ਰੁੱਖ(ਝੂਠ));
        Ast_ਗੰਢ_Module* ਅਚਲ resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ਅਚਲ ਤਾਰਕਿਕ ਮੇਲ = resolvedModp && VN_CAST(resolvedModp, Class) == ਵਿਅੰਜਕ_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_ਰੁੱਖ(), origClonep);
        m_default_Clone_ਵਿੱਚ_ਤਰੱਕੀ.erase(origClassRefp->classp());
        ਵਾਪਸ ਮੇਲ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ param_Consts_Equal_At_Max_ਚੌੜਾਈ(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        ਜੇ (exprp->ਅੰਕ().ਚੌੜਾਈ() == origp->ਅੰਕ().ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (exprp->ਅੰਕ().isOpaque()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (origp->ਅੰਕ().isOpaque()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਪੂਰਨ max_ਚੌੜਾਈ = std::max(exprp->ਅੰਕ().ਚੌੜਾਈ(), origp->ਅੰਕ().ਚੌੜਾਈ());
        V3_ਸੰਖਿਆ ਵਿਅੰਜਕ_ਅੰਕ{exprp, max_ਚੌੜਾਈ};
        ਜੇ (exprp->ਅੰਕ().isSigned()) {
            ਵਿਅੰਜਕ_ਅੰਕ.opExtendS(exprp->ਅੰਕ(), exprp->ਅੰਕ().ਚੌੜਾਈ());
        } ਨਹੀਂਤਾਂ {
            ਵਿਅੰਜਕ_ਅੰਕ.op_ਸੌਂਪੋ(exprp->ਅੰਕ());
        }
        V3_ਸੰਖਿਆ orig_ਅੰਕ{origp, max_ਚੌੜਾਈ};
        ਜੇ (origp->ਅੰਕ().isSigned()) {
            orig_ਅੰਕ.opExtendS(origp->ਅੰਕ(), origp->ਅੰਕ().ਚੌੜਾਈ());
        } ਨਹੀਂਤਾਂ {
            orig_ਅੰਕ.op_ਸੌਂਪੋ(origp->ਅੰਕ());
        }
        V3_ਸੰਖਿਆ isEq{exprp, 1};
        isEq.opEq(ਵਿਅੰਜਕ_ਅੰਕ, orig_ਅੰਕ);
        ਵਾਪਸ isEq.isNeqZero();
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
    ਖਾਲੀ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (!pinp->exprp()) ਜਾਰੀ;
            ਜੇ (!VString::startsWith(pinp->name(), "__VGIfaceParam")) ਜਾਰੀ;
            ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ fwdRefp = VN_CAST(pinp->exprp(), ਗੰਢ_Var_Ref);
            ਜੇ (!fwdRefp) ਜਾਰੀ;
            ਅਚਲ AstIfaceRefDType* ਅਚਲ resolvedp
                = VN_CAST(fwdRefp->varp()->ਬੱਚਾ_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ਅਚਲ newIrefp = ਨਵਾਂ AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            ਜੇ (resolvedp->paramsp()) {
                newIrefp->ਜੋੜੋ_Paramsp(resolvedp->paramsp()->clone_ਰੁੱਖ(ਸੱਚ));
            }
            pinp->exprp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    ਖਾਲੀ name_By_Pin_ਮੁੱਲ(AstPin* pinp, Ast_ਗੰਢ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        ਤਾਰਕਿਕ& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_ਮੁੱਲ_ਸੰਖਿਆ(pinp->exprp());
        any_overridesr = ਸੱਚ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    ਖਾਲੀ cellPinCleanup(Ast_ਗੰਢ* nodep, AstPin* pinp, AstPin* paramsp, Ast_ਗੰਢ_Module* srcModp,
                        string& longnamer, ਤਾਰਕਿਕ& any_overridesr,
                        std::unordered_ਸੈੱਟ<ਅਚਲ AstPin*>& overridingTypePinsr) {
        ਜੇ (!pinp->exprp()) ਵਾਪਸ;  // No-connect
        ਜੇ (AstVar* ਅਚਲ modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            ਜੇ (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(pinp->exprp(), ਨਮੂਨਾ)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_ਮੁੱਲ(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } ਨਹੀਂਤਾਂ {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                ਜੇ (is_Aggregate_Param_ਮੁੱਲ(pinp->exprp())) {
                    name_By_Pin_ਮੁੱਲ(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    ਵਾਪਸ;
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
                ਜੇ (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_ਮਾਪ))) {
                    ਅਚਲ ਤਾਰਕਿਕ dependent = modvarp->valuep()->exists(
                        [](Ast_ਗੰਢ* np) { ਵਾਪਸ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    ਜੇ (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                ਜੇ (isString(modvarp->subDTypep())) {
                    convert_ਮੰਜ਼ਿਲ_Stringp(pinp->exprp());
                    convert_ਮੰਜ਼ਿਲ_Stringp(modvarp->valuep());
                }
                AstConst* ਅਚਲ exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ਅਚਲ origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (exprp && !exprp->ਅੰਕ().isDouble() && !exprp->ਅੰਕ().isString()) {
                    AstVar* cloneVarp = modvarp->clone_ਰੁੱਖ(ਝੂਠ);
                    ਤਾਰਕਿਕ cloneVarpUnresolved = ਝੂਠ;
                    ਜੇ (Ast_ਗੰਢ* ਅਚਲ oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_ਪਿੱਠ();
                        VL_DO_DANGLING(oldValuep->delete_ਰੁੱਖ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_ਰੁੱਖ(ਝੂਠ));
                    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        ਜੇ (cloneVarp->ਬੱਚਾ_DTypep())
                            cloneVarp->ਬੱਚਾ_DTypep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                        cloneVarp->ਬੱਚਾ_DTypep(origDTypep->clone_ਰੁੱਖ(ਝੂਠ));
                        cloneVarp->dtypep(ਖਾਲੀ_ਸੂਚਕ);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr ਪੂਰਨ maxSubstIters = 1000;
                        ਲਈ (ਪੂਰਨ it = 0; it < maxSubstIters; ++it) {
                            ਤਾਰਕਿਕ any = ਝੂਠ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ਅਚਲ targetp = varrefp->varp();
                                Ast_ਗੰਢ* replacep = ਖਾਲੀ_ਸੂਚਕ;
                                ਲਈ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ਜੇ (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        ਜੇ (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), ਨਮੂਨਾ)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_ਰੁੱਖ(ਝੂਠ);
                                        }
                                        ਰੋਕੋ;
                                    }
                                }
                                ਜੇ (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_ਰੁੱਖ(ਝੂਠ);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_ਨਮੂਨਾ* ਅਚਲ patp = VN_CAST(replacep, ਨਮੂਨਾ);
                                ਜੇ (patp && !patp->ਬੱਚਾ_DTypep() && targetp->ਬੱਚਾ_DTypep()) {
                                    patp->ਬੱਚਾ_DTypep(targetp->ਬੱਚਾ_DTypep()->clone_ਰੁੱਖ(ਝੂਠ));
                                }
                                ਜੇ (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_ਰੁੱਖ(), varrefp);
                                    any = ਸੱਚ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::ਸਦਿਸ਼<std::pair<AstRefDType*, Ast_ਗੰਢ_DType*>> ਮੰਜ਼ਿਲ_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ਅਚਲ ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                ਜੇ (!ptdp) ਵਾਪਸ;
                                AstPin* overridePinp = ਖਾਲੀ_ਸੂਚਕ;
                                ਲਈ (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    ਜੇ (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        ਰੋਕੋ;
                                    }
                                }
                                Ast_ਗੰਢ_DType* ਅਚਲ substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ਗੰਢ_DType)
                                                   : ptdp->subDTypep();
                                ਜੇ (substp) ਮੰਜ਼ਿਲ_Replace.emplace_ਪਿੱਠ(refp, substp);
                            });
                            ਲਈ (ਸਵੈਚਾਲਿਤ it = ਮੰਜ਼ਿਲ_Replace.rbegin(); it != ਮੰਜ਼ਿਲ_Replace.rend(); ++it) {
                                AstRefDType* ਅਚਲ refp = it->ਪਹਿਲਾ;
                                refp->replaceWith(it->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
                                VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
                                any = ਸੱਚ;
                            }
                            ਜੇ (!any) ਰੋਕੋ;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ਗੰਢ* np) {
                            ਜੇ (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = ਸੱਚ;
                                ਵਾਪਸ;
                            }
                            ਜੇ (Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ ਵਸਤੂ_Refp = VN_CAST(np, Enum_ਵਸਤੂ_Ref)) {
                                ਜੇ (V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(ਵਸਤੂ_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = ਸੱਚ;
                                }
                                ਵਾਪਸ;
                            }
                            AstRefDType* ਅਚਲ refp = VN_CAST(np, RefDType);
                            ਜੇ (!refp) ਵਾਪਸ;
                            ਜੇ (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = ਸੱਚ;
                            }
                            // A type declared in the template must not be widthed there
                            ਜੇ (V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = ਸੱਚ;
                            }
                        });
                    }
                    ਜੇ (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        ਜੇ (AstConst* ਅਚਲ widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            ਜੇ (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_ਪਿੱਠ();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_ਰੁੱਖ(), cloneVarp);
                }
                AstConst* ਅਚਲ namingExprp = normedNamep ? normedNamep : exprp;
                ਜੇ (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ਗੰਢ* ਅਚਲ pinExprp = pinp->exprp();
                    pinExprp->replaceWith(ਨਵਾਂ AstConst{pinp->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                                       modvarp->ਚੌੜਾਈ(), 0});
                    VL_DO_DANGLING(pinExprp->delete_ਰੁੱਖ(), pinExprp);
                } ਨਹੀਂਤਾਂ ਜੇ (origp
                           && (exprp->same_ਰੁੱਖ(origp)
                               || (exprp->ਅੰਕ().ਚੌੜਾਈ() == origp->ਅੰਕ().ਚੌੜਾਈ()
                                   && Parameterized_Hier_ਖੰਡਾਂ::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_ਚੌੜਾਈ(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } ਨਹੀਂਤਾਂ ਜੇ (namingExprp->ਅੰਕ().isDouble() || namingExprp->ਅੰਕ().isString()
                           || namingExprp->ਅੰਕ().isFourState()
                           || namingExprp->ਅੰਕ().ਚੌੜਾਈ() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_ਮੁੱਲ_ਸੰਖਿਆ(namingExprp));
                    any_overridesr = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->ਅੰਕ().ascii(ਝੂਠ));
                    any_overridesr = ਸੱਚ;
                }
                ਜੇ (normedNamep) VL_DO_DANGLING(normedNamep->delete_ਰੁੱਖ(), normedNamep);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ਗੰਢ_DType* rawTypep = VN_CAST(pinp->exprp(), ਗੰਢ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            ਤਾਰਕਿਕ skip_ਚੌੜਾਈ_For_Template_Struct = ਝੂਠ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ਗੰਢ_DType* ਅਚਲ resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ਗੰਢ_Module* ਅਚਲ ownerModp
                        = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    ਜੇ (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_ਚੌੜਾਈ_For_Template_Struct = ਸੱਚ;
                        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                ਜੇ (rawTypep && !skip_ਚੌੜਾਈ_For_Template_Struct) V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(rawTypep);
            }
            Ast_ਗੰਢ_DType* exprp = rawTypep ? rawTypep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp() : ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Ast_ਗੰਢ_DType* origp = modvarp->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp();
            ਜੇ (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } ਨਹੀਂਤਾਂ {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                ਜੇ (!skip_ਚੌੜਾਈ_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ਗੰਢ_DType);
                    exprp = rawTypep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp();
                }
                ਜੇ (!modvarp->fwdType().is_ਗੰਢ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                ਜੇ (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (!skip_ਚੌੜਾਈ_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ਗੰਢ_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp() : ਖਾਲੀ_ਸੂਚਕ;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    ਜੇ (AstClassRefDType* ਅਚਲ classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        ਜੇ (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } ਨਹੀਂਤਾਂ {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    ਖਾਲੀ cellInterfaceCleanup(AstPin* pinsp, Ast_ਗੰਢ_Module* srcModp, string& longnamer,
                              ਤਾਰਕਿਕ& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        ਲਈ (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਅਚਲ AstVar* ਅਚਲ modvarp = pinp->modVarp();
            ਜੇ (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                ਜਾਰੀ;
            }
            ਜੇ (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ਅਚਲ ਪੋਰਟ_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ exprp = pinp->exprp();
                ਜੇ (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    ਜਾਰੀ;
                }
                AstIfaceRefDType* pinIrefp = ਖਾਲੀ_ਸੂਚਕ;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrp = VN_CAST(exprp, ਗੰਢ_Var_Ref)) {
                    ਜੇ (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                ਜੇ (!pinIrefp && exprp) {
                    ਅਚਲ Ast_ਗੰਢ* basep = exprp;
                    ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_Pre_Sel* ਅਚਲ selp = VN_CAST(basep, ਗੰਢ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vrp = VN_CAST(basep, VarRef)) {
                        ਜੇ (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << ਪੋਰਟ_Irefp);

                ਜੇ (!ਪੋਰਟ_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } ਨਹੀਂਤਾਂ ਜੇ (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } ਨਹੀਂਤਾਂ {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    ਜੇ (ਪੋਰਟ_Irefp->iface_ਰਾਹੀਂ_Cellp() != pinIrefp->iface_ਰਾਹੀਂ_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_ਮੁੱਲ_ਸੰਖਿਆ(pinIrefp));
                        any_overridesr = ਸੱਚ;
                        ifaceRefRefs.emplace_ਪਿੱਠ(ਪੋਰਟ_Irefp, pinIrefp);
                        ਜੇ (ਪੋਰਟ_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && ਪੋਰਟ_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ਗੰਢ::prettyNameQ(ਪੋਰਟ_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ਗੰਢ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    ਖਾਲੀ storeOriginalParams(Ast_ਗੰਢ_Module* ਅਚਲ classp) {
        ਲਈ (Ast_ਗੰਢ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ਗੰਢ* originalParamp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isGParam()) originalParamp = varp->clonep();
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                ਜੇ (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            ਜੇ (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    ਖਾਲੀ generic_Interface_Var_ਤਿਆਰੀ(ਅਚਲ AstPin* ਅਚਲ paramsp, ਅਚਲ AstPin* ਅਚਲ pinsp) {
        std::unordered_map<string, ਅਚਲ AstPin*> paramspMap;
        ਲਈ (ਅਚਲ AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.ਪਾਓ({pinp->name().substr(std::ਲੜੀ_ਲੰਬਾਈ("__VGIfaceParam")), pinp});
            }
        }

        ਜੇ (paramspMap.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstPin* ਅਚਲ pinp = VN_CAST(nodep, Pin)) {
                ਜੇ (AstVar* ਅਚਲ varp = pinp->modVarp()) {
                    Ast_ਗੰਢ_DType* dtypep = varp->ਬੱਚਾ_DTypep()->elemDTypep();
                    ਜੇ (AstIfaceGenericDType* ਅਚਲ ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = paramspMap.ਲੱਭੋ(varp->name());
                        ਜੇ (iter == paramspMap.ਸਿਰਾ()) ਜਾਰੀ;
                        Ast_ਗੰਢ* ਅਚਲ backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_ਪਿੱਠ();
                        ਅਚਲ AstPin* ਅਚਲ paramp = iter->ਦੂਜਾ;
                        paramspMap.erase(iter);
                        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ਅਚਲ newIfacerefp = ਨਵਾਂ AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ arrDtp = VN_CAST(backp, ਗੰਢ_Array_DType)) {
                            arrDtp->ਬੱਚਾ_DTypep(newIfacerefp);
                        } ਨਹੀਂਤਾਂ ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->ਬੱਚਾ_DTypep(newIfacerefp);
                        } ਨਹੀਂਤਾਂ {
                            varp->ਬੱਚਾ_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ਧੱਕੋ_Deletep(ifaceGDTypep), ifaceGDTypep);
                        ਜੇ (paramspMap.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
                    }
                }
            }
        }

        UASSERT(paramspMap.ਖਾਲੀ_ਥਾਵਾਂ(), "Not every generic interface implicit param is used");
    }

    ਖਾਲੀ resolveDefaultParams(Ast_ਗੰਢ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        AstPin* paramsp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            ਜੇ (!classp) ਵਾਪਸ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } ਨਹੀਂਤਾਂ ਜੇ (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_ਸੂਚਕਾਂਕ.clear();
        ਲਈ (Ast_ਗੰਢ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                ਜੇ (!paramTypep->isGParam()) ਜਾਰੀ;
                m_param_ਸੂਚਕਾਂਕ.emplace(paramTypep, static_cast<ਪੂਰਨ>(m_classTypeParams.ਮਾਪ()));
                m_classTypeParams.emplace_ਪਿੱਠ(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_DType*> defaultTypeNodes(m_classTypeParams.ਮਾਪ(), ਖਾਲੀ_ਸੂਚਕ);
        ਲਈ (ਮਾਪ_t i = 0; i < m_classTypeParams.ਮਾਪ(); ++i) {
            AstParamTypeDType* ਅਚਲ paramTypep = m_classTypeParams[i].ਪਹਿਲਾ;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ਅਚਲ reqDtp = VN_CAST(paramTypep->get_ਬੱਚਾ_DTypep(), RequireDType);
            ਜੇ (!reqDtp) ਜਾਰੀ;

            // If default is a reference to another param type, record dependency
            ਜੇ (AstRefDType* ਅਚਲ refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                ਜੇ (AstParamTypeDType* ਅਚਲ ਸਰੋਤ_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    ਸਵੈਚਾਲਿਤ it = m_param_ਸੂਚਕਾਂਕ.ਲੱਭੋ(ਸਰੋਤ_Paramp);
                    ਜੇ (it != m_param_ਸੂਚਕਾਂਕ.ਸਿਰਾ()) { m_classTypeParams[i].ਦੂਜਾ = it->ਦੂਜਾ; }
                    ਜਾਰੀ;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(reqDtp->lhsp(), ਗੰਢ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::ਸਦਿਸ਼<AstPin*> pins_By_ਸੂਚਕਾਂਕ;
        pins_By_ਸੂਚਕਾਂਕ.resize(m_classTypeParams.ਮਾਪ(), ਖਾਲੀ_ਸੂਚਕ);
        ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ਅਚਲ typep = pinp->modPTypep();
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = typep ? m_param_ਸੂਚਕਾਂਕ.ਲੱਭੋ(typep) : m_param_ਸੂਚਕਾਂਕ.ਸਿਰਾ();
            ਜੇ (it != m_param_ਸੂਚਕਾਂਕ.ਸਿਰਾ()) {
                pins_By_ਸੂਚਕਾਂਕ[it->ਦੂਜਾ] = pinp;
            } ਨਹੀਂਤਾਂ {
                pins_By_ਸੂਚਕਾਂਕ.ਧੱਕੋ_ਪਿੱਠ(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        ਲਈ (ਤਾਰਕਿਕ resolvedSome = ਸੱਚ; resolvedSome;) {
            resolvedSome = ਝੂਠ;
            ਲਈ (ਮਾਪ_t paramIdx = 0; paramIdx < m_classTypeParams.ਮਾਪ(); paramIdx++) {
                ਜੇ (pins_By_ਸੂਚਕਾਂਕ[paramIdx]) ਜਾਰੀ;
                ਅਚਲ ਪੂਰਨ ਸਰੋਤ_Param_Idx = m_classTypeParams[paramIdx].ਦੂਜਾ;

                AstPin* newPinp = ਖਾਲੀ_ਸੂਚਕ;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                ਜੇ (ਸਰੋਤ_Param_Idx >= 0 && pins_By_ਸੂਚਕਾਂਕ[ਸਰੋਤ_Param_Idx]) {
                    newPinp = pins_By_ਸੂਚਕਾਂਕ[ਸਰੋਤ_Param_Idx]->clone_ਰੁੱਖ(ਝੂਠ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                ਜੇ (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = defaultTypeNodes[paramIdx];
                    newPinp = ਨਵਾਂ AstPin{dtypep->fileline(), static_cast<ਪੂਰਨ>(paramIdx) + 1,
                                         "__paramNumber" + cvt_ਮੰਜ਼ਿਲ_Str(paramIdx + 1),
                                         dtypep->clone_ਰੁੱਖ(ਝੂਠ)};
                }

                ਜੇ (newPinp) {
                    newPinp->name("__paramNumber" + cvt_ਮੰਜ਼ਿਲ_Str(paramIdx + 1));
                    newPinp->param(ਸੱਚ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].ਪਹਿਲਾ);
                    ਜੇ (classOrPackageRef) {
                        classOrPackageRef->ਜੋੜੋ_Paramsp(newPinp);
                    } ਨਹੀਂਤਾਂ ਜੇ (classRefDType) {
                        classRefDType->ਜੋੜੋ_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_ਸੂਚਕਾਂਕ[paramIdx] = newPinp;
                    ਜੇ (!paramsp) paramsp = newPinp;
                    resolvedSome = ਸੱਚ;
                }
            }
        }
    }

    Ast_ਗੰਢ_Module* ਗੰਢ_Deparam_Common(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, ਤਾਰਕਿਕ any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        ਜੇ (debug() >= 9 && paramsp) paramsp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-  cellparams: ");

        ਜੇ (srcModp->hier_ਖੰਡ()) {
            longname = parameterized_Hier_ਖੰਡ_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } ਨਹੀਂਤਾਂ {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_ਸੈੱਟ<ਅਚਲ AstPin*> overridingTypePins;
            ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (!pinp->exprp()) ਜਾਰੀ;  // An empty override keeps the default
                ਜੇ (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    ਜਾਰੀ;
                }
                AstVar* ਅਚਲ modvarp = pinp->modVarp();
                ਜੇ (!modvarp || !modvarp->isGParam()) ਜਾਰੀ;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ਅਚਲ Ast_ਨਮੂਨਾ* ਅਚਲ patternp = VN_CAST(pinp->exprp(), ਨਮੂਨਾ);
                ਜੇ (patternp && !patternp->ਬੱਚਾ_DTypep()) ਜਾਰੀ;
                ਜੇ (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_ਮੁੱਲ(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (AstParamTypeDType* ਅਚਲ modptp = pinp->modPTypep()) {
                    ਜੇ (!overridingTypePins.ਗਿਣਤੀ_2(pinp)) ਜਾਰੀ;
                    Ast_ਗੰਢ_DType* ਅਚਲ typep = VN_AS(pinp->exprp(), ਗੰਢ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_ਮੁੱਲ_ਸੰਖਿਆ(typep->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp());
                    any_overrides = ਸੱਚ;
                    ਜਾਰੀ;
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
        ਤਾਰਕਿਕ defaultsResolved = ਝੂਠ;
        ਜੇ (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            ਲਈ (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                ਜੇ (pinp->modPTypep()) {
                    any_overrides = ਸੱਚ;
                    defaultsResolved = ਸੱਚ;
                    ਰੋਕੋ;
                }
            }
        }
        ਜੇ (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.ਮੰਜ਼ਿਲ_String();
            any_overrides = ਸੱਚ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ਗੰਢ_Module* newModp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (m_hier_ਖੰਡਾਂ.hier_Sub_ਲੜੀਵਾਰ() && m_hier_ਖੰਡਾਂ.is_Hier_ਖੰਡ(srcModp->origName())) {
            Ast_ਗੰਢ_Module* ਅਚਲ paramedModp
                = m_hier_ਖੰਡਾਂ.ਲੱਭੋ_By_Params(srcModp->origName(), paramsp, m_modp);
            ਜੇ (!paramedModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Unsupported, already reported
            paramedModp->dead(ਝੂਠ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } ਨਹੀਂਤਾਂ ਜੇ (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            ਜੇ (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ਗੰਢ_Module* ਗੰਢ_Copyp = srcModp->clone_ਰੁੱਖ(ਝੂਠ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ਗੰਢ_Copyp);
                m_deleter.ਧੱਕੋ_Deletep(ਗੰਢ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ਗੰਢ_Copyp);
                storeOriginalParams(ਗੰਢ_Copyp);
            }
            newModp = srcModp;
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ string newname
                = srcModp->hier_ਖੰਡ() ? longname : moduleCalcName(srcModp, longname);
            ਅਚਲ ModInfo* ਅਚਲ modInfop
                = module_ਲੱਭੋ_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            ਜੇ (!modInfop) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ਅਚਲ ਤਾਰਕਿਕ cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ਅਚਲ newClassp = VN_CAST(newModp, Class);
        ਜੇ (newClassp && !newClassp->user2()) m_specializedClassps.ਧੱਕੋ_ਪਿੱਠ(newClassp);

        // Link source class to its specialized version for later relinking of method references
        ਜੇ (defaultsResolved) srcModp->user4p(newModp);

        ਲਈ (ਸਵੈਚਾਲਿਤ* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                ਜੇ (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_ਤਿਆਰੀ(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        ਜੇ (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ(), paramsp);
        ਵਾਪਸ newModp;
    }

    Ast_ਗੰਢ_Module* cellDeparam(AstCell* nodep, Ast_ਗੰਢ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ਗੰਢ_Module* ਅਚਲ newModp = ਗੰਢ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        ਜੇ (!newModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(ਝੂਠ);
        ਵਾਪਸ newModp;
    }
    Ast_ਗੰਢ_Module* ifaceRefDeparam(AstIfaceRefDType* ਅਚਲ nodep, Ast_ਗੰਢ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        ਜੇ (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ਅਚਲ string ownerOrigName
                = ownerIfacep->origName().ਖਾਲੀ_ਥਾਵਾਂ() ? ownerIfacep->name() : ownerIfacep->origName();
            ਅਚਲ string srcOrigName
                = srcModp->origName().ਖਾਲੀ_ਥਾਵਾਂ() ? srcModp->name() : srcModp->origName();
            string owner_ਅਧਾਰ_Name = ownerOrigName;
            ਅਚਲ ਮਾਪ_t ownerPos = owner_ਅਧਾਰ_Name.ਲੱਭੋ("__");
            ਜੇ (ownerPos != string::npos) owner_ਅਧਾਰ_Name = owner_ਅਧਾਰ_Name.substr(0, ownerPos);
            string src_ਅਧਾਰ_Name = srcOrigName;
            ਅਚਲ ਮਾਪ_t srcPos = src_ਅਧਾਰ_Name.ਲੱਭੋ("__");
            ਜੇ (srcPos != string::npos) src_ਅਧਾਰ_Name = src_ਅਧਾਰ_Name.substr(0, srcPos);

            ਜੇ (owner_ਅਧਾਰ_Name == src_ਅਧਾਰ_Name) {
                ਤਾਰਕਿਕ allOwnParams = ਸੱਚ;
                ਲਈ (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    ਜੇ (AstRefDType* ਅਚਲ refp = VN_CAST(pinp->exprp(), RefDType)) {
                        ਜੇ (AstParamTypeDType* ਅਚਲ ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ਗੰਢ_Module* ਅਚਲ ptdOwnerp
                                = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(ptdp);
                            ਜੇ (ptdOwnerp != m_modp) allOwnParams = ਝੂਠ;
                        } ਨਹੀਂਤਾਂ {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } ਨਹੀਂਤਾਂ {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                ਜੇ (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    ਜੇ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
                    ਵਾਪਸ ownerIfacep;
                }
            }
        }

        Ast_ਗੰਢ_Module* ਅਚਲ newModp
            = ਗੰਢ_Deparam_Common(nodep, srcModp, nodep->paramsp(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ);
        ਜੇ (!newModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        nodep->ifacep(VN_AS(newModp, Iface));
        ਵਾਪਸ newModp;
    }
    Ast_ਗੰਢ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ਗੰਢ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ਗੰਢ_Module* ਅਚਲ newModp
            = ਗੰਢ_Deparam_Common(nodep, srcModp, nodep->paramsp(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ);
        ਜੇ (!newModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ਅਚਲ refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ਅਚਲ newClassp = refDTypep ? VN_CAST(newModp, Class) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            ਜੇ (AstTypedef* ਅਚਲ typedefp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        ਵਾਪਸ newModp;
    }
    Ast_ਗੰਢ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ਗੰਢ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ਗੰਢ_Module* ਅਚਲ newModp
            = ਗੰਢ_Deparam_Common(nodep, srcModp, nodep->paramsp(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ);
        ਜੇ (!newModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstClass* ਅਚਲ newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        ਵਾਪਸ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    ਵਰਗ DeferredResolverVisitor final : ਜਨਤਕ VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ*> m_reachedDecls;

        ਤਾਰਕਿਕ ਪਹਿਲਾ_ਪਹੁੰਚ(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ declp) { ਵਾਪਸ m_reachedDecls.ਪਾਓ(declp).ਦੂਜਾ; }

        ਖਾਲੀ iterateParamType(AstParamTypeDType* ਅਚਲ paramTypep) {
            ਜੇ (ਪਹਿਲਾ_ਪਹੁੰਚ(paramTypep)) iterate(paramTypep);
        }
        ਖਾਲੀ iterateTypedef(AstTypedef* ਅਚਲ tdefp) {
            ਜੇ (ਪਹਿਲਾ_ਪਹੁੰਚ(tdefp)) iterate(tdefp->subDTypep());
        }

        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
        ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ਗੰਢ* ਅਚਲ targetp = nodep->classOrPackageNodep();
            ਜੇ (AstTypedef* ਅਚਲ tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_ਮੰਜ਼ਿਲ_Typedef(nodep);
        }
        ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ਅਚਲ tdefp = nodep->typedefp();
            ਜੇ (tdefp) {
                iterateTypedef(tdefp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ਅਚਲ varp = nodep->varp();
            ਜੇ (ਪਹਿਲਾ_ਪਹੁੰਚ(varp)) {
                iterate(varp->subDTypep());
                ਅਚਲ ਸਵੈਚਾਲਿਤ& deferredVarps = v3Global.rootp()->deferredParamVarps();
                ਜੇ (varp->varType() == VVarType::LPARAM && deferredVarps.ਗਿਣਤੀ_2(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    ਜਨਤਕ:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ਗੰਢ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

ਜਨਤਕ:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    ਖਾਲੀ retargetIfaceRefs(Ast_ਗੰਢ_Module* ਮਾਪਾ_Modp, ਅਚਲ string& cellName) {
        Ast_ਗੰਢ_Module* ਅਚਲ correctModp
            = V3LinkDotIfaceCapture::follow_Cell_ਰਸਤਾ(ਮਾਪਾ_Modp, cellName);
        ਜੇ (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) ਵਾਪਸ;
        ਮਾਪਾ_Modp->foreach([&](AstRefDType* refp) {
            ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = V3LinkDotIfaceCapture::capture_ਟੈਗ(refp);
            ਜੇ (!tagp || tagp->m_cell_ਰਸਤਾ != cellName) ਵਾਪਸ;
            // Only retarget REFDTYPEs owned by parentModp.
            ਜੇ (V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(refp) != ਮਾਪਾ_Modp) ਵਾਪਸ;
            ਜੇ (V3LinkDotIfaceCapture::retarget_Ref_ਮੰਜ਼ਿਲ_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    ਖਾਲੀ resolveDeferredDotsReachableFrom(Ast_ਗੰਢ* rootp, ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*ਇਹ, rootp};
    }

    Ast_ਗੰਢ_Module* ਗੰਢ_Deparam(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_Module* srcModp, Ast_ਗੰਢ_Module* modp,
                               ਅਚਲ string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ਗੰਢ::ਗੰਢ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ਗੰਢ_Name = nodep->name();
        ਜੇ (AstIfaceRefDType* ਅਚਲ ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            ਜੇ (ਗੰਢ_Name.ਖਾਲੀ_ਥਾਵਾਂ()) ਗੰਢ_Name = ifaceRefp->cellName();
        }
        ਅਚਲ string instanceName
            = ਗੰਢ_Name.ਖਾਲੀ_ਥਾਵਾਂ() ? someInstanceName : (someInstanceName + "." + ਗੰਢ_Name);

        ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
            ਲਈ (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ਅਚਲ nextp = VN_AS(pinp->nextp(), Pin);
                ਜੇ (!pinp->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ() && instanceName != pinp->param_ਰਸਤਾ()
                    && !VString::ਸਿਰੇ_With(instanceName, "." + pinp->param_ਰਸਤਾ())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ਗੰਢ::ਗੰਢ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ਗੰਢ_Module* newModp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstIfaceRefDType* ਅਚਲ ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassRefDType* ਅਚਲ classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassOrPackageRef* ਅਚਲ classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        ਜੇ (!newModp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ਗੰਢ::ਗੰਢ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        ਵਾਪਸ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::ਸਦਿਸ਼<AstClass*> takeSpecializedClassps() {
        std::ਸਦਿਸ਼<AstClass*> ਲਿਆ;
        ਲਿਆ.ਅਦਲਾ_ਬਦਲੀ(m_specializedClassps);
        ਵਾਪਸ ਲਿਆ;
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ParamProcessor(AstNetlist* nodep)
        : m_hier_ਖੰਡਾਂ{v3Global.opt.hier_ਖੰਡਾਂ(), nodep} {
        ਲਈ (Ast_ਗੰਢ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
            m_all_Module_ਨਾਂ.ਪਾਓ(modp->name());
        }
    }
    ~ParamProcessor() = ਮੂਲ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

ਵਰਗ ClassRefUnlinkerVisitor final : ਜਨਤਕ VNVisitor {

ਜਨਤਕ:
    ਸਪਸ਼ਟ ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        ਜੇ (nodep->paramsp()) {
            ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                ਜੇ (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ਅਚਲ dotp = VN_CAST(nodep->backp(), Dot);
                    ਜੇ (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_ਪਿੱਠ());
                        VL_DO_DANGLING2(ਧੱਕੋ_Deletep(dotp), dotp, nodep);
                    } ਨਹੀਂਤਾਂ {
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {}  // don't iterate inside classes
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

ਵਰਗ ParamClassRefDTypeRelinkVisitor final : ਜਨਤਕ VNVisitor {

    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;
    std::unordered_map<ਅਚਲ AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ਗੰਢ_Module* m_ownerModp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_Module* m_ownerMapForp = ਖਾਲੀ_ਸੂਚਕ;
    std::unordered_map<std::string, AstClass*> m_orig_Name_ਮੰਜ਼ਿਲ_Clone;
    std::unordered_ਸੈੱਟ<std::string> m_ambiguous;

    ਖਾਲੀ ensureOwnerMap() {
        ਜੇ (m_ownerMapForp == m_ownerModp) ਵਾਪਸ;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_ਮੰਜ਼ਿਲ_Clone.clear();
        m_ambiguous.clear();
        ਜੇ (!m_ownerModp) ਵਾਪਸ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        ਜੇ (AstClass* ਅਚਲ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ਜੇ (ownerClassp->hasGParam()) ਵਾਪਸ;
        }
        ਲਈ (Ast_ਗੰਢ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ਅਚਲ tdp = VN_CAST(sp, Typedef);
            ਜੇ (!tdp) ਜਾਰੀ;
            AstClassRefDType* ਅਚਲ crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            ਜੇ (!crdtp || !crdtp->classp()) ਜਾਰੀ;
            AstClass* ਅਚਲ targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            ਜੇ (targetp->hasGParam()) ਜਾਰੀ;
            ਅਚਲ std::string origName
                = targetp->origName().ਖਾਲੀ_ਥਾਵਾਂ() ? targetp->name() : targetp->origName();
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_orig_Name_ਮੰਜ਼ਿਲ_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            ਜੇ (!pair.ਦੂਜਾ && pair.ਪਹਿਲਾ->ਦੂਜਾ != targetp) m_ambiguous.ਪਾਓ(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ਸਥਿਰ string classOrigName(ਅਚਲ AstClass* classp) {
        ਵਾਪਸ classp->origName().ਖਾਲੀ_ਥਾਵਾਂ() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ਸਥਿਰ AstTypedef* ਲੱਭੋ_Typedef_With_Bases(AstClass* classp, ਅਚਲ string& name) {
        ਲਈ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            ਜੇ (AstTypedef* ਅਚਲ tdp = V3LinkDotIfaceCapture::ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(cp, name)) {
                ਜੇ (tdp->subDTypep()) ਵਾਪਸ tdp;
            }
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    ਖਾਲੀ retargetRefDType(AstRefDType* refp) {
        ਜੇ (!m_ownerModp) ਵਾਪਸ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        ਜੇ (AstClass* ਅਚਲ ownerClassp = VN_CAST(m_ownerModp, Class)) {
            ਜੇ (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ਅਚਲ AstClass* refClassp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (AstTypedef* ਅਚਲ tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(tdp), Class);
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassOrPackageRef* ਅਚਲ classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                ਜੇ (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ਅਚਲ selfTdp = ਲੱਭੋ_Typedef_With_Bases(ownerClassp, refp->name());
                    ਜੇ (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        ਵਾਪਸ;
                    }
                }
            }
        }

        AstTypedef* ਅਚਲ oldTdp = refp->typedefp();
        ਜੇ (!oldTdp) ਵਾਪਸ;
        AstClass* ਅਚਲ oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(oldTdp), Class);
        ਜੇ (!oldOwnerp) ਵਾਪਸ;

        ensureOwnerMap();
        ਜੇ (m_orig_Name_ਮੰਜ਼ਿਲ_Clone.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਅਚਲ std::string origName
            = oldOwnerp->origName().ਖਾਲੀ_ਥਾਵਾਂ() ? oldOwnerp->name() : oldOwnerp->origName();
        ਜੇ (m_ambiguous.ਗਿਣਤੀ_2(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            ਵਾਪਸ;
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_orig_Name_ਮੰਜ਼ਿਲ_Clone.ਲੱਭੋ(origName);
        ਜੇ (it == m_orig_Name_ਮੰਜ਼ਿਲ_Clone.ਸਿਰਾ()) ਵਾਪਸ;
        AstClass* ਅਚਲ correctClassp = it->ਦੂਜਾ;
        ਜੇ (correctClassp == oldOwnerp) ਵਾਪਸ;
        AstTypedef* ਅਚਲ newTdp
            = V3LinkDotIfaceCapture::ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(correctClassp, refp->name());
        ਜੇ (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            ਵਾਪਸ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        ਜੇ (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            ਵਾਪਸ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstParamTypeDType* ਅਚਲ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        ਜੇ (AstParamTypeDType* ਅਚਲ paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_paramTypeClassMap.ਲੱਭੋ(paramtypep);
            ਜੇ (it != m_paramTypeClassMap.ਸਿਰਾ()) {
                AstClass* ਅਚਲ origClassp = it->ਦੂਜਾ;
                ਜੇ (origClassp->hasGParam() && !origClassp->user3p()) {
                    ਜੇ (AstClass* ਅਚਲ parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ਅਚਲ string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = ਖਾਲੀ_ਸੂਚਕ;
                        ਲਈ (Ast_ਗੰਢ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ਜੇ (AstParamTypeDType* ਅਚਲ ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                ਜੇ (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    ਰੋਕੋ;
                                }
                            }
                        }
                        ਜੇ (newParamTypep) {
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
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

ਵਰਗ ParamState final {
ਜਨਤਕ:
    // STATE - across all visitors
    std::ਸਦਿਸ਼<AstClass*> m_paramClasses;  // Parameterized classes
    std::ਸਦਿਸ਼<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    ਵਰਤੋਂ WQKey = std::pair<ਤਾਰਕਿਕ, ਪੂਰਨ>;
    std::multimap<WQKey, Ast_ਗੰਢ_Module*> m_work_ਕਤਾਰ_ਅਗਲਾ;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ਗੰਢ_Module*, std::unordered_ਸੈੱਟ<Ast_ਗੰਢ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

ਵਰਗ ParamVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    ਤਾਰਕਿਕ m_iterateModule = ਝੂਠ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<ਤਾਰਕਿਕ, Ast_ਗੰਢ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, std::string>
        m_gen_Hier_ਨਾਂ;  // Maps ast nodes to generated hierarchy names
    std::map<ਅਚਲ AstRefDType*, ਤਾਰਕਿਕ>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    ਵਰਤੋਂ VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Module iterating
    std::unordered_ਸੈੱਟ<std::string> m_iface_ਪੋਰਟ_ਨਾਂ;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    ਤਾਰਕਿਕ m_mod_Iface_Refs_ਪੂਰਾ = ਝੂਠ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Generate_Cond = ਝੂਠ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    ਖਾਲੀ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ਗੰਢ_Module*> work_ਕਤਾਰ;
        m_generateHierName = "";
        m_iterateModule = ਸੱਚ;

        // Visit all cells under module, recursively
        ਜਦੋਂ (ਸੱਚ) {
            // Classes specialized since the last pass still need their bodies elaborated
            ਲਈ (AstClass* ਅਚਲ classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_ਕਤਾਰ_ਅਗਲਾ.emplace(ParamState::WQKey{ਸੱਚ, classp->level()}, classp);
            }
            ਜੇ (work_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) std::ਅਦਲਾ_ਬਦਲੀ(work_ਕਤਾਰ, m_state.m_work_ਕਤਾਰ_ਅਗਲਾ);
            ਜੇ (work_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) ਰੋਕੋ;

            ਅਚਲ ਸਵੈਚਾਲਿਤ itm = work_ਕਤਾਰ.cbegin();
            Ast_ਗੰਢ_Module* ਅਚਲ modp = itm->ਦੂਜਾ;
            work_ਕਤਾਰ.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            ਜੇ (!modp->user2_ਸੈੱਟ_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                ਜੇ (modp->someInstanceName().ਖਾਲੀ_ਥਾਵਾਂ()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_ਪੋਰਟ_ਨਾਂ);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_ਪੂਰਾ);
                    m_modp = modp;
                    m_mod_Iface_Refs_ਪੂਰਾ = ਝੂਠ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            ਜਦੋਂ (!m_cellps.ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ itim = m_cellps.cbegin();
                Ast_ਗੰਢ* ਅਚਲ cellp = itim->ਦੂਜਾ;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ਅਚਲ ਸਵੈਚਾਲਿਤ genHierIt = m_gen_Hier_ਨਾਂ.ਲੱਭੋ(cellp);
                ਜੇ (genHierIt != m_gen_Hier_ਨਾਂ.ਸਿਰਾ()) {
                    genHierName = std::move(genHierIt->ਦੂਜਾ);
                    m_gen_Hier_ਨਾਂ.erase(genHierIt);
                }

                Ast_ਗੰਢ_Module* srcModp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (ਅਚਲ AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    ਜੇ (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } ਨਹੀਂਤਾਂ {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                ਜੇ (!srcModp) ਜਾਰੀ;

                // Update path
                ਅਚਲ string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ newModp
                    = m_processor.ਗੰਢ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    ਜੇ (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        ਜੇ (V3LinkDotIfaceCapture::enabled()) {
                            ਜੇ (ਅਚਲ AstCell* ਅਚਲ modCellp = VN_CAST(cellp, Cell)) {
                                ਜੇ (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_ਕਤਾਰ.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].ਪਾਓ(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    ਜੇ (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = ਝੂਠ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ਸਥਿਰ string get_Ref_ਅਧਾਰ_Name(ਅਚਲ AstVarXRef* refp) {
        ਅਚਲ string dotted = refp->dotted();
        ਜੇ (dotted.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ "";
        ਵਾਪਸ dotted.substr(0, dotted.ਲੱਭੋ('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    ਖਾਲੀ logTemplateLeakRefs(Ast_ਗੰਢ_Module* ਮਾਪਾ_Modp, Ast_ਗੰਢ_Module* templateModp,
                             ਅਚਲ ਅੱਖਰ* stage, Ast_ਗੰਢ* contextp) {
        ਜੇ (debug() < 9 || !ਮਾਪਾ_Modp || !templateModp || !VN_IS(templateModp, Iface)) ਵਾਪਸ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        ਪੂਰਨ leak_ਗਿਣਤੀ = 0;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ancestry_ਦਾ = [](ਅਚਲ Ast_ਗੰਢ* nodep) {
            string ancestry;
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* curp = nodep; curp; curp = curp->backp()) {
                ਜੇ (!ancestry.ਖਾਲੀ_ਥਾਵਾਂ()) ancestry += "<-";
                ancestry += curp->typeName();
                ਜੇ (VN_IS(curp, ਗੰਢ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_ਸਾਰਣੀ)) {
                    ਰੋਕੋ;
                }
            }
            ਵਾਪਸ ancestry;
        };

        ਮਾਪਾ_Modp->foreach([&](AstRefDType* refp) {
            ਜੇ (refp->typedefp()) {
                Ast_ਗੰਢ_Module* ਅਚਲ tdOwnerp
                    = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(refp->typedefp());
                ਜੇ (tdOwnerp == templateModp) {
                    ++leak_ਗਿਣਤੀ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << ਮਾਪਾ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ਗੰਢ::ਗੰਢ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_ਦਾ(refp));
                }
            }
            ਜੇ (refp->refDTypep()) {
                Ast_ਗੰਢ_Module* ਅਚਲ rdOwnerp
                    = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(refp->refDTypep());
                ਜੇ (rdOwnerp == templateModp) {
                    ++leak_ਗਿਣਤੀ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << ਮਾਪਾ_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ਗੰਢ::ਗੰਢ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_ਦਾ(refp));
                }
            }
        });

        ਮਾਪਾ_Modp->foreach([&](AstVarRef* varrefp) {
            ਜੇ (!varrefp->varp()) ਵਾਪਸ;
            Ast_ਗੰਢ_Module* ਅਚਲ varOwnerp
                = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(varrefp->varp());
            ਜੇ (varOwnerp != templateModp) ਵਾਪਸ;
            ++leak_ਗਿਣਤੀ;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << ਮਾਪਾ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ਗੰਢ::ਗੰਢ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_ਦਾ(varrefp));
        });

        ਜੇ (leak_ਗਿਣਤੀ > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << ਮਾਪਾ_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ਗਿਣਤੀ);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    ਖਾਲੀ checkParamNotHierRecurse(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ underTypeQuery = ਝੂਠ) {
        ਲਈ (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ਅਚਲ Ast_Attr_ਦਾ* ਅਚਲ attrp = VN_CAST(nodep, Attr_ਦਾ);
            ਅਚਲ ਤਾਰਕਿਕ ਬੱਚਾ_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            ਜੇ (ਅਚਲ AstVarXRef* ਅਚਲ refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                ਤਾਰਕਿਕ isIfaceRef = ਝੂਠ;
                ਜੇ (refp->varp() && refp->varp()->isIfaceParam()) {
                    ਅਚਲ string refname = get_Ref_ਅਧਾਰ_Name(refp);
                    isIfaceRef
                        = !refname.ਖਾਲੀ_ਥਾਵਾਂ()
                          && (m_iface_ਪੋਰਟ_ਨਾਂ.ਗਿਣਤੀ_2(refname) || m_ifaceInstCells.ਗਿਣਤੀ_2(refname));
                }
                ਜੇ (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_FTask_Ref* ਅਚਲ refp = VN_CAST(nodep, ਗੰਢ_FTask_Ref)) {
                ਜੇ (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), ਬੱਚਾ_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), ਬੱਚਾ_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), ਬੱਚਾ_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), ਬੱਚਾ_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    ਖਾਲੀ specializeNestedIfaceCells(Ast_ਗੰਢ_Module* ifaceModp) {
        ਲਈ (Ast_ਗੰਢ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ਅਚਲ nestedCellp = VN_CAST(stmtp, Cell);
            ਜੇ (!nestedCellp) ਜਾਰੀ;
            ਜੇ (!VN_IS(nestedCellp->modp(), Iface)) ਜਾਰੀ;
            ਜੇ (!nestedCellp->paramsp()) ਜਾਰੀ;

            Ast_ਗੰਢ_Module* ਅਚਲ nestedSrcModp = nestedCellp->modp();
            ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ nestedNewModp = m_processor.ਗੰਢ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                ਜੇ (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    ਲਈ (Ast_ਗੰਢ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(sp, Var)) {
                            ਜੇ (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    ਜੇ (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    ਤਾਰਕਿਕ cellParamsReferenceIfacePorts(AstCell* cellp) {
        ਜੇ (!cellp->paramsp()) ਵਾਪਸ ਝੂਠ;

        ਲਈ (Ast_ਗੰਢ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (AstPin* ਅਚਲ pinp = VN_CAST(nodep, Pin)) {
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ exprp = pinp->exprp()) {
                    ਜੇ (ਅਚਲ AstVarXRef* ਅਚਲ refp = VN_CAST(exprp, VarXRef)) {
                        ਅਚਲ string refname = get_Ref_ਅਧਾਰ_Name(refp);
                        ਜੇ (!refname.ਖਾਲੀ_ਥਾਵਾਂ()
                            && (m_iface_ਪੋਰਟ_ਨਾਂ.ਗਿਣਤੀ_2(refname)
                                || m_ifaceInstCells.ਗਿਣਤੀ_2(refname)))
                            ਵਾਪਸ ਸੱਚ;
                    }
                }
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    ਖਾਲੀ expand_Iface_Array_ਪੋਰਟ_Dimensions(AstVar* portp, Ast_ਗੰਢ_DType* dtypep,
                                        ਅਚਲ std::string& suffix) {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        ਜੇ (!arrp) {
            AstVar* ਅਚਲ varp = portp->clone_ਰੁੱਖ(ਝੂਠ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ oldp = varp->ਬੱਚਾ_DTypep()) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(oldp->unlink_Fr_ਪਿੱਠ()), oldp);
            }
            varp->dtypep(ਖਾਲੀ_ਸੂਚਕ);
            varp->ਬੱਚਾ_DTypep(portp->subDTypep()->elemDTypep()->clone_ਰੁੱਖ(ਝੂਠ));
            portp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp);
            ਵਾਪਸ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ਲਈ (ਅਚਲ ਪੂਰਨ n : arrp->declRange().seq_ਸੱਜਾ_ਮੰਜ਼ਿਲ_ਖੱਬਾ()) {
            ਅਚਲ std::string s = suffix + "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(n) + "__KET__";
            expand_Iface_Array_ਪੋਰਟ_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    ਖਾਲੀ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ਅਚਲ AstRange* rangep, ਅਚਲ std::string& suffix, ਪੂਰਨ idx) {
        // Base case: insert the element when no dimensions left
        ਜੇ (!rangep) {
            AstCell* ਅਚਲ elemp = arrayedCellp->clone_ਰੁੱਖ(ਝੂਠ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(elemp);
            ਜੇ (ifaceVarp) {
                AstIfaceRefDType* ਅਚਲ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_ਰੁੱਖ(ਝੂਠ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ਅਚਲ varp = ifaceVarp->clone_ਰੁੱਖ(ਝੂਠ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ oldp = varp->ਬੱਚਾ_DTypep()) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(oldp->unlink_Fr_ਪਿੱਠ()), oldp);
                }
                varp->dtypep(ਖਾਲੀ_ਸੂਚਕ);
                varp->ਬੱਚਾ_DTypep(irefp);
                ifaceVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp);
            }
            ਵਾਪਸ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ਅਚਲ AstRange* ਅਚਲ subRangep = VN_AS(rangep->nextp(), Range);
        ਲਈ (ਅਚਲ ਪੂਰਨ n : rangep->seq_ਸੱਜਾ_ਮੰਜ਼ਿਲ_ਖੱਬਾ()) {
            ਅਚਲ std::string s = suffix + "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    ਖਾਲੀ ਦੇਖੋ_Cell_Or_Class_Ref(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_ਨਾਂ.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        ਜੇ (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell);
            ਜੇ (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ਗੰਢ_Module* ਅਚਲ srcModp = cellp->modp();
                Ast_ਗੰਢ_Module* ਅਚਲ newModp
                    = m_processor.ਗੰਢ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                ਜੇ (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                ਜੇ (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        ਜੇ (!m_ਵਿੱਚ_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->recursiveClone()) nodep->dead(ਸੱਚ);  // Fake, made for recursive elimination
        ਜੇ (nodep->dead()) ਵਾਪਸ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            ਜੇ (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ਧੱਕੋ_ਪਿੱਠ(classp);
                ਵਾਪਸ;
            }
        }

        ਜੇ (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_ਕਤਾਰ_ਅਗਲਾ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            ਵਾਪਸ;
        }

        // Start traversal at root-like things
        ਜੇ (nodep->is_ਸਿਖਰ()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_ਕਤਾਰ_ਅਗਲਾ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    ਤਾਰਕਿਕ isCircularType(ਅਚਲ AstRefDType* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_isCircular.emplace(nodep, ਸੱਚ);
        ਜੇ (!iter.ਦੂਜਾ) ਵਾਪਸ iter.ਪਹਿਲਾ->ਦੂਜਾ;
        ਜੇ (ਅਚਲ AstRefDType* ਅਚਲ subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ਅਚਲ ਤਾਰਕਿਕ ret = isCircularType(subDTypep);
            iter.ਪਹਿਲਾ->ਦੂਜਾ = ret;
            ਵਾਪਸ ret;
        }
        iter.ਪਹਿਲਾ->ਦੂਜਾ = ਝੂਠ;
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        ਅਚਲ ਤਾਰਕਿਕ isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        ਜੇ (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        ਜੇ (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_ਸੈੱਟ_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        ਜੇ (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            ਲਈ (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ਅਚਲ ifaceVarp = [&]() -> AstVar* {
                ਜੇ (!VN_IS(nodep->modp(), Iface)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ਅਚਲ varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                ਵਾਪਸ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            ਜੇ (ifaceVarp) {
                ifaceVarp->is_Iface_Array_ਵੰਡੋ(ਸੱਚ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ਅਚਲ firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ਅਚਲ irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            ਵਾਪਸ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        ਜੇ (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        ਦੇਖੋ_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_ਸੈੱਟ_Once();
        ਜੇ (nodep->ifacep()) ਦੇਖੋ_Cell_Or_Class_Ref(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_ਸੈੱਟ_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        ਦੇਖੋ_Cell_Or_Class_Ref(nodep, ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        ਜੇ (!VN_IS(nodep->classOrPackageNodep(), Typedef)) ਦੇਖੋ_Cell_Or_Class_Ref(nodep, ਝੂਠ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ਸਥਿਰ ਖਾਲੀ gatherVars(Ast_ਗੰਢ* stmtsp, ਤਾਰਕਿਕ (*matchp)(ਅਚਲ AstVar*), VarsByName& varsr) {
        ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                ਜੇ (matchp(varp)) varsr.emplace(varp->name(), varp);
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_Gen_ਖੰਡ* ਅਚਲ genp = VN_CAST(nodep, Gen_ਖੰਡ)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ਅਚਲ VarsByName& modIfaceRefs() {
        ਜੇ (!m_mod_Iface_Refs_ਪੂਰਾ) {
            gatherVars(
                m_modp->stmtsp(), [](ਅਚਲ AstVar* varp) { ਵਾਪਸ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_ਪੂਰਾ = ਸੱਚ;
        }
        ਵਾਪਸ m_modIfaceRefs;
    }
    ਅਚਲ VarsByName& ifaceParams(Ast_ਗੰਢ_Module* ifacep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        ਜੇ (pair.ਦੂਜਾ) {
            gatherVars(
                ifacep->stmtsp(), [](ਅਚਲ AstVar* varp) { ਵਾਪਸ varp->isParam(); },
                pair.ਪਹਿਲਾ->ਦੂਜਾ);
        }
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ਗੰਢ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ਅਚਲ lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ਅਚਲ rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        ਜੇ (!lhsp || !rhsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        ਜੇ (VL_UNCOVERABLE(!m_modp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        ਅਚਲ ਸਵੈਚਾਲਿਤ& refVars = modIfaceRefs();
        ਅਚਲ ਸਵੈਚਾਲਿਤ ifaceVarIt = refVars.ਲੱਭੋ(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        ਜੇ (VL_UNCOVERABLE(ifaceVarIt == refVars.ਸਿਰਾ())) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVar* ਅਚਲ ifaceVarp = ifaceVarIt->ਦੂਜਾ;
        AstIfaceRefDType* ਅਚਲ ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        ਜੇ (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        ਅਚਲ ਸਵੈਚਾਲਿਤ& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਨਿਸ਼ਾਨਾ_Var_It = ifaceVars.ਲੱਭੋ(rhsp->name());
        ਜੇ (ਨਿਸ਼ਾਨਾ_Var_It == ifaceVars.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVar* ਅਚਲ ਨਿਸ਼ਾਨਾ_Varp = ਨਿਸ਼ਾਨਾ_Var_It->ਦੂਜਾ;
        // processWorkQ() visits interface cells first, so the interface is already constified
        ਜੇ (VL_UNCOVERABLE(!VN_IS(ਨਿਸ਼ਾਨਾ_Varp->valuep(), Const))) iterate(ਨਿਸ਼ਾਨਾ_Varp);
        ਜੇ (VL_UNCOVERABLE(!ਨਿਸ਼ਾਨਾ_Varp->valuep() || !VN_IS(ਨਿਸ਼ਾਨਾ_Varp->valuep(), Const))) {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // LCOV_EXCL_LINE
        }
        ਵਾਪਸ ਨਿਸ਼ਾਨਾ_Varp->valuep()->clone_ਰੁੱਖ(ਝੂਠ);
    }
    ਖਾਲੀ resolve_Generic_Iface_Dots_ਵਿੱਚ(Ast_ਗੰਢ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::ਸਦਿਸ਼<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ਧੱਕੋ_ਪਿੱਠ(dotp); });
        ਲਈ (AstDot* ਅਚਲ dotp : dotps) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_ਰੁੱਖ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (nodep->user2_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        // Interface array port: split it into its elements
        ਜੇ (nodep->isIfaceRef() && !nodep->is_Iface_ਮਾਪਾ()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_ਪੋਰਟ_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_ਵੰਡੋ(ਸੱਚ);
        }
        // Build cache of interface port names as we encounter them
        ਜੇ (nodep->isIfaceRef()) { m_iface_ਪੋਰਟ_ਨਾਂ.ਪਾਓ(nodep->name()); }
        iterateChildren(nodep);
        ਜੇ (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            ਜੇ (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_ਵਿੱਚ(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                ਤਾਰਕਿਕ hasUnresolvedLparamXRef = ਝੂਠ;
                nodep->valuep()->foreach([&](ਅਚਲ AstVarXRef* xrefp) {
                    ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = xrefp->varp()) {
                        ਜੇ (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = ਸੱਚ;
                        }
                    }
                });
                ਜੇ (hasUnresolvedLparamXRef) ਵਾਪਸ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ਅਚਲ ਤਾਰਕਿਕ hasDot = nodep->valuep()->exists([](AstDot*) { ਵਾਪਸ ਸੱਚ; });
                ਤਾਰਕਿਕ refsDeferred = ਝੂਠ;
                ਜੇ (!hasDot) {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ਅਚਲ AstVarRef* refp) {
                        ਜੇ (refsDeferred) ਵਾਪਸ;
                        AstVar* ਅਚਲ refVarp = refp->varp();
                        ਜੇ (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ਗਿਣਤੀ_2(refVarp)) {
                            refsDeferred = ਸੱਚ;
                        }
                    });
                }
                ਜੇ (hasDot || refsDeferred) {
                    v3Global.rootp()->ਧੱਕੋ_Deferred_Param_Varp(nodep);
                    ਵਾਪਸ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        ਜੇ (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        ਜੇ (nodep->varp()) iterate(nodep->varp());
    }
    ਤਾਰਕਿਕ ifaceParamReplace(AstVarXRef* nodep, Ast_ਗੰਢ* candp) {
        ਲਈ (; candp; candp = candp->nextp()) {
            ਜੇ (nodep->name() == candp->name()) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    ਜੇ (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    ਵਾਪਸ ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPin* ਅਚਲ pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_ਰੁੱਖ(ਝੂਠ));
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ deparamIfaceCellNow(AstCell* cellp) {
        ਜੇ (!cellp->paramsp()) ਵਾਪਸ;
        ਜੇ (!VN_IS(cellp->modp(), Iface)) ਵਾਪਸ;
        Ast_ਗੰਢ_Module* ਅਚਲ srcModp = cellp->modp();
        Ast_ਗੰਢ_Module* ਅਚਲ newModp
            = m_processor.ਗੰਢ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        ਜੇ (newModp && newModp != srcModp) {
            ਜੇ (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (nodep->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(ਖਾਲੀ_ਸੂਚਕ);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        ਜੇ (nodep->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ()) nodep->itemp(ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        ਜੇ (nodep->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(ਖਾਲੀ_ਸੂਚਕ);
            ਵਾਪਸ;
        }
        // Parameters of an interface array are accessed through its first element
        ਜੇ (nodep->varp() && nodep->varp()->isParam()) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ifaceInstCells.ਲੱਭੋ(nodep->dotted());
            ਜੇ (it != m_ifaceInstCells.ਸਿਰਾ()) nodep->dotted(it->ਦੂਜਾ->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ਅਚਲ string dotted = nodep->dotted();
        ਜੇ (!dotted.ਖਾਲੀ_ਥਾਵਾਂ() && nodep->varp() && nodep->varp()->isParam()) {
            ਅਚਲ Ast_ਗੰਢ* backp = nodep;
            ਜਦੋਂ ((backp = backp->backp())) {
                ਜੇ (VN_IS(backp, ਗੰਢ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    ਰੋਕੋ;
                }
                ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(backp, Var)) {
                    ਜੇ (!varp->isIfaceRef()) ਜਾਰੀ;
                    // Through all dimensions of an array
                    ਅਚਲ AstIfaceRefDType* ਅਚਲ ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    ਜੇ (!ifacerefp) ਜਾਰੀ;
                    // Interfaces passed in on the port map have ifaces
                    ਜੇ (ਅਚਲ AstIface* ਅਚਲ ifacep = ifacerefp->ifacep()) {
                        ਜੇ (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            ਜੇ (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                ਵਾਪਸ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    ਨਹੀਂਤਾਂ ਜੇ (AstCell* ਅਚਲ cellp = ifacerefp->cellp()) {
                        ਜੇ (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            ਜੇ (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                ਵਾਪਸ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ਅਚਲ ਸਵੈਚਾਲਿਤ ifaceCellIt = m_ifaceInstCells.ਲੱਭੋ(dotted);
            ਜੇ (ifaceCellIt != m_ifaceInstCells.ਸਿਰਾ()) {
                AstCell* ਅਚਲ cellp = ifaceCellIt->ਦੂਜਾ;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ਅਚਲ AstClass* lhsClassp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstClassOrPackageRef* ਅਚਲ classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        ਜੇ (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ਗੰਢ* rhsDefp = ਖਾਲੀ_ਸੂਚਕ;
        AstClassOrPackageRef* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        ਜੇ (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        ਜੇ (lhsClassp && rhsDefp) {
            m_state.m_dots.ਧੱਕੋ_ਪਿੱਠ(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } ਨਹੀਂਤਾਂ {
            iterate(nodep->rhsp());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstUnlinkedRef* nodep) override {
        AstVarXRef* ਅਚਲ varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ਗੰਢ_FTask_Ref* ਅਚਲ taskrefp = VN_CAST(nodep->refp(), ਗੰਢ_FTask_Ref);
        Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ enumrefp = VN_CAST(nodep->refp(), Enum_ਵਸਤੂ_Ref);
        ਜੇ (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } ਨਹੀਂਤਾਂ ਜੇ (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } ਨਹੀਂਤਾਂ ਜੇ (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            ਵਾਪਸ;
        }
        iterate(nodep->cellrefp());

        ਜੇ (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } ਨਹੀਂਤਾਂ ਜੇ (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } ਨਹੀਂਤਾਂ {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        ਲਈ (Ast_ਗੰਢ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::ਸਦਿਸ਼<ਪੂਰਨ> indices;
        ਲਈ (Ast_ਗੰਢ* s = nodep->selp(); s; s = s->nextp()) {
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(s, Const);
            ਜੇ (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                ਵਾਪਸ;
            }
            indices.ਧੱਕੋ_ਪਿੱਠ(constp->ਮੰਜ਼ਿਲ_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ਅਚਲ string viftopSuffix = "__Viftop";
        ਅਚਲ string ਅਧਾਰ_Name
            = VString::ਸਿਰੇ_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().ਮਾਪ() - viftopSuffix.ਮਾਪ())
                  : nodep->name();
        ਅਚਲ string placeholder = "__BRA__??__KET__";
        ਮਾਪ_t pos = m_unlinkedTxt.ਲੱਭੋ(ਅਧਾਰ_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        ਜੇ (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            ਵਾਪਸ;
        }
        pos += ਅਧਾਰ_Name.ਲੰਮਾਈ();
        ਲਈ (ਪੂਰਨ idx : indices) {
            ਅਚਲ string replacement = "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(idx) + "__KET__";
            ਜੇ (m_unlinkedTxt.compare(pos, placeholder.ਲੰਮਾਈ(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                ਵਾਪਸ;
            }
            m_unlinkedTxt.replace(pos, placeholder.ਲੰਮਾਈ(), replacement);
            pos += replacement.ਲੰਮਾਈ();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    ਖਾਲੀ iterateGenerateCond(Ast_ਗੰਢ* nodep) {
        VL_RESTORER(m_ਵਿੱਚ_Generate_Cond);
        m_ਵਿੱਚ_Generate_Cond = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    ਖਾਲੀ ਦੇਖੋ(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->condp(), Const)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                nodep->replaceWith(keepp);
            } ਨਹੀਂਤਾਂ {
                nodep->unlink_Fr_ਪਿੱਠ();
            }
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            // Normal edit rules will now recurse the replacement
        } ਨਹੀਂਤਾਂ {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        ਜੇ (AstGenFor* ਅਚਲ forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ਅਚਲ string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        ਤਾਰਕਿਕ ਪ੍ਰਾਪਤੀ = ਝੂਠ;
        Ast_ਗੰਢ* keepp = ਖਾਲੀ_ਸੂਚਕ;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        ਲਈ (Ast_Gen_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ਵਸਤੂ)) {
            ਲਈ (Ast_ਗੰਢ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ਅਚਲ AstConst* ਅਚਲ exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        ਲਈ (Ast_Gen_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ਵਸਤੂ)) {
            ਲਈ (Ast_ਗੰਢ* ep = itemp->condsp(); ep;) {
                Ast_ਗੰਢ* ਅਚਲ nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        ਲਈ (Ast_Gen_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ਵਸਤੂ)) {
            ਜੇ (!itemp->isDefault()) {
                ਲਈ (Ast_ਗੰਢ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    ਜੇ (ਅਚਲ AstConst* ਅਚਲ ccondp = VN_CAST(ep, Const)) {
                        V3_ਸੰਖਿਆ ਮੇਲ{nodep, 1};
                        ਮੇਲ.opEq(ccondp->ਅੰਕ(), exprp->ਅੰਕ());
                        ਜੇ (!ਪ੍ਰਾਪਤੀ && ਮੇਲ.isNeqZero()) {
                            ਪ੍ਰਾਪਤੀ = ਸੱਚ;
                            keepp = itemp->itemsp();
                        }
                    } ਨਹੀਂਤਾਂ {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        ਲਈ (Ast_Gen_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_ਵਸਤੂ)) {
            ਜੇ (itemp->isDefault()) {
                ਜੇ (!ਪ੍ਰਾਪਤੀ) {
                    ਪ੍ਰਾਪਤੀ = ਸੱਚ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        ਜੇ (keepp) {
            keepp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            nodep->replaceWith(keepp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = ਮੂਲ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

ਵਰਗ Param_ਸਿਖਰ final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    ਖਾਲੀ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        ਲਈ (AstDot* ਅਚਲ dotp : m_state.m_dots) {
            ਅਚਲ AstClassOrPackageRef* ਅਚਲ classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ਅਚਲ AstClass* ਅਚਲ lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ਅਚਲ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            ਲਈ (ਸਵੈਚਾਲਿਤ* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    ਰੋਕੋ;
                }
            }
        }
    }

    ਖਾਲੀ fixLevel(Ast_ਗੰਢ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        ਜੇ (modp->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Already fixed
        ਜੇ (m_state.m_parentps[modp].ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;  // Leave top levels alone
        ਪੂਰਨ max_ਮਾਪਾ_Level = 0;
        ਲਈ (Ast_ਗੰਢ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_ਮਾਪਾ_Level = std::max(max_ਮਾਪਾ_Level, parentp->level());
        }
        ਜੇ (modp->level() <= max_ਮਾਪਾ_Level) modp->level(max_ਮਾਪਾ_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->ਡੂੰਘਾਈ(modp->level());
    }

    ਖਾਲੀ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Module*> modps;
        ਲਈ (Ast_ਗੰਢ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ਗੰਢ_Module);
            modp->unlink_Fr_ਪਿੱਠ();
            modps.ਧੱਕੋ_ਪਿੱਠ(modp);
        }

        // Fix-up levels
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        ਲਈ (Ast_ਗੰਢ_Module* ਅਚਲ modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_ਛਾਂਟੋ(modps.begin(), modps.ਸਿਰਾ(),
                         [](ਅਚਲ Ast_ਗੰਢ_Module* ap, ਅਚਲ Ast_ਗੰਢ_Module* bp) {
                             ਵਾਪਸ ap->level() < bp->level();
                         });

        // Re-insert modules
        ਲਈ (Ast_ਗੰਢ_Module* ਅਚਲ modp : modps) netlistp->ਜੋੜੋ_Modulesp(modp);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ Param_ਸਿਖਰ(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ਅਚਲ ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ਅਚਲ ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ਗੰਢ_FTask_Ref* ftaskrefp) {
            Ast_ਗੰਢ_FTask* ftaskp = ftaskrefp->taskp();
            ਜੇ (!ftaskp || !ftaskp->class_ਵਿਧੀ()) ਵਾਪਸ;
            ਅਚਲ std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ਅਚਲ ਸਵੈਚਾਲਿਤ ancestor_Class_ਦਾ = [](Ast_ਗੰਢ* startp) -> AstClass* {
                ਲਈ (Ast_ਗੰਢ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    ਜੇ (AstClass* ਅਚਲ cp = VN_CAST(np, Class)) ਵਾਪਸ cp;
                }
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            };
            AstClass* refClassp = ancestor_Class_ਦਾ(ftaskrefp);
            ਜੇ (refClassp == ftaskrefp->classOrPackagep())
                ਵਾਪਸ;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_ਦਾ(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            ਜੇ (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                ਵਾਪਸ;
            }
            ਜੇ (classp->user3p()) ਵਾਪਸ;  // will not get removed, no need to relink
            AstClass* ਅਚਲ parametrizedClassp = VN_CAST(classp->user4p(), Class);
            ਜੇ (!parametrizedClassp) ਵਾਪਸ;
            Ast_ਗੰਢ_FTask* newFuncp = ਖਾਲੀ_ਸੂਚਕ;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ਗੰਢ_FTask* ftaskp) {
                ਜੇ (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    ਵਾਪਸ ਸੱਚ;
                }
                ਵਾਪਸ ਝੂਠ;
            });
            ਜੇ (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ਅਚਲ ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_ਸੈੱਟ<AstClass*> removedClassps;
        ਲਈ (AstClass* ਅਚਲ classp : m_state.m_paramClasses) {
            ਜੇ (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                ਜੇ (V3Error::error_ਗਿਣਤੀ()) ਜਾਰੀ;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(classp->unlink_Fr_ਪਿੱਠ()), classp);
            } ਨਹੀਂਤਾਂ {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(ਝੂਠ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ਗੰਢ::user3_Clear_ਰੁੱਖ();
        ਲਈ (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ਗੰਢ* ਅਚਲ nodep) { nodep->user3(ਸੱਚ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ਗੰਢ* ਅਚਲ nodep) {
            nodep->foreachLink([&](Ast_ਗੰਢ** ਅਚਲ linkpp, ਅਚਲ ਅੱਖਰ* namep) {
                ਜੇ (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = ਖਾਲੀ_ਸੂਚਕ;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_ਸਿਖਰ() = ਮੂਲ;
    VL_UNCOPYABLE(Param_ਸਿਖਰ);
};

//######################################################################
// Param class functions

ਖਾਲੀ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    ਜੇ (dump_ਰੁੱਖ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_ਸਿਖਰ{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    ਜੇ (dump_ਰੁੱਖ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_ਰੁੱਖ("param", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    ਲਈ (AstVar* ਅਚਲ varp : rootp->deferredParamVarps()) {
        ਜੇ (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
