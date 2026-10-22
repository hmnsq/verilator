// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split arrays and structs into separate variables
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
// V3Decompose replaces variables of aggregate type (unpacked array, unpacked
// struct, multidimensional packed array, or packed struct) with one variable
// per element or member, called a component. This avoids false combinational
// loops (UNOPTFLAT) through the parts of a variable, and lets downstream passes
// optimize each component separately. Note specifically that one dimensional
// packed arrays (vectors, e.g. logic [31:0]) are not split into individual
// bits by this pass.
//
// Components that are themselves aggregates can be split further. The pass
// decides for the variable, and for each of its components at any depth,
// whether to split it.
//
// The pass proceeds in three steps:
//   - DecomposeRecord walks the netlist and records necessary information about
//     accesses to aggregate variables, without changing the tree.
//   - DecomposeDecision decides which variables and components to split, in one
//     iterative analysis of the data structures recorded by DecomposeRecord.
//   - DecomposeRewrite replaces references to, and expands assignments between
//     split variables and components, in a single rewrite across all constructs
//     involved.
// This structure ensures there is only a single traversal of the netlist, the
// complete splitting decision is made globally, independent of iteration order,
// and the rewriting is done after all decisions are made, so there is no need
// to re-analyze, or keep track of stale references anywhere.
//
// The core concept and data structure that enables the decomposition of the
// three steps is the 'place'. A 'place' represents a possible storage location:
// - either an original variable,
// - or one of its components, recursively.
// 'places' form a tree, with the original variable as the root. For example,
// the tree of 'places' involved for the aggregate variable 's' are:
//
//   struct packed {
//     logic [1:0][3:0] m;
//     logic [2:0] f;
//   } s [2];
//
//   s                      'place' for the unpacked array of 2 packed structs
//   +-- s[0]               'place' for element 0 of 's', a packed struct
//   |   +-- s[0].m         'place' for member 'm' of 's[0]', a packed array of 2 elements
//   |   |   +-- s[0].m[0]  'place' logic [3:0], a non splittable leaf
//   |   |   +-- s[0].m[1]  'place' logic [3:0], a non splittable leaf
//   |   +-- s[0].f         'place' logic [2:0], a non splittable leaf
//   +-- s[1]               Same subtree structure as 's[0]'
//       +-- s[1].m
//       |   +-- s[1].m[0]
//       |   +-- s[1].m[1]
//       +-- s[1].f
//
// A place is automatically split if it can be, and it wants to be.
// It can be split if all of the following are true:
//   - it is an aggregate variable
//   - the original variable is eligible for splitting (not public, etc.)
//   - its parent is split (if it is a component)
//   - it is not blocked, by being referenced other than in splittable constructs
// It wants to be split if any of the following are true:
//   - one of its components is addressed by a constant select
//   - the original variable is marked with split_var
//   - a copy assignment between it and another split place exists
// Note specifically that a reference outside a constant indexed select, or
// a copy assignment will block splitting, so effectively the variable is split
// mostly if it is only accessed via constant indexed selects, and it's only
// assigned whole (recursively for its components).
//
// E.g. this will split 'arr' into 'arr[0]' and 'arr[1]':
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[1];  // All indices are constant, so 'arr' is split
//
// This will not split 'arr', as it is indexed with a non-constant index:
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[i];  // 'i' is not a constant, so 'arr' is not split
//
// Copy assignments propagate splitting. An assignment between two places is
// recorded as a copy. When a place is split, its copies are split at the
// boundaries of its components. For example:
//
//   typedef struct packed {
//     logic [3:0] hi;
//     logic [3:0] lo;
//   } pair_t;
//   pair_t a, b;
//   assign b = {a.lo + 4'd1, in};
//   assign a = b;
//   assign out = a.hi;
//
// 'a' is split, as its components are selected. This splits the 'a = b' copy
// into copies between 'a.hi' and 'b[7:4]', and 'a.lo' and 'b[3:0]', each covering
// only one component of 'b', so 'b' is split too. The result is a list of
// assignments between the components of 'a' and 'b':
//
//   assign b__DOT__hi = a__DOT__lo + 4'd1;
//   assign b__DOT__lo = in;
//   assign a__DOT__hi = b__DOT__hi;
//   assign a__DOT__lo = b__DOT__lo;
//   assign out = a__DOT__hi;
//
// Which later passes can simplify into 'out = in + 4'd1', with intermediate
// variables discarded when possible.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Decompose.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

తరగతి Decompose_పునాది VL_NOT_FINAL {
రక్షిత:
    // TYPES

    // A component (element or member) of an aggregate type
    నిర్మాణం Component final {
        Ast_కణుపు_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        పూర్ణాంకం declIdx;  // The declared index of the element, if an array
        పూర్ణాంకం lsb;  // LSB in the whole value, if packed
        పూర్ణాంకం msb;  // MSB in the whole value, if packed
    };

    నిర్మాణం ఉంచు;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    నిర్మాణం Copy final {
        ఉంచు* otherp;  // The Place at the other end
        // Following only for packed places
        పూర్ణాంకం lsb;  // The first bit covered of this end
        పూర్ణాంకం otherLsb;  // The first bit covered of the other end
        పూర్ణాంకం వెడల్పు;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    నిర్మాణం ఉంచు final {
        ఉంచు* parentp = శూన్యసూచిక;  // The Place this is a component of, nullptr if original
        ఉంచు* rootp = శూన్యసూచిక;  // The original variable (root of the tree), root points to itself
        Ast_కణుపు_DType* dtypep = శూన్యసూచిక;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = శూన్యసూచిక;
        // The component Places created so far, by index, sized on the first one created
        std::సదిశ<std::unique_ptr<ఉంచు>> childrenp;
        std::సదిశ<Copy> copies;  // The copies covering it, until split
        తార్కిక splittable = నిజం;  // Can be split (there is no reason not to)
        తార్కిక కోరుతుంది_విభజించు = అబద్ధం;  // Should be split (if possible, do split)
        తార్కిక విభజించు = అబద్ధం;  // Place was split
        తార్కిక queued = అబద్ధం;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    నిర్మాణం Select final {
        Ast_కణుపు_సమాసం* exprp;  // The select
        ఉంచు* placep;  // The component
        పూర్ణాంకం lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    నిర్మాణం Assignment final {
        Ast_కణుపు_కేటాయించు* assp;  // The assignment
        ఉంచు* lPlacep;  // The Place the LHS addresses exactly, if any
        ఉంచు* rPlacep;  // The Place the RHS addresses exactly, if any
    };

బహిరంగ:
    // The state of the pass, shared by the steps
    తరగతి State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        స్థిరరాశి VNUser4_లోపల_Use m_user4_లోపల_Use;

    బహిరంగ:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_కణుపు_DType, std::సదిశ<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, ఉంచు> places;
        std::సదిశ<ఉంచు*> rootps;  // The original Places, in creation order
        std::సదిశ<std::సదిశ<Select>> chains;  // Select chains addressing components
        std::సదిశ<Assignment> assignments;  // Assignments to expand if a side is split
    };

రక్షిత:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    స్పష్టమైన Decompose_పునాది(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    స్థిర తార్కిక isUnpacked(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (VN_IS(dtypep, UnpackArrayDType)) తిరిగి నిజం;
        స్థిరరాశి AstStructDType* స్థిరరాశి structp = VN_CAST(dtypep, StructDType);
        తిరిగి structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    స్థిర తార్కిక isPacked(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, PackArrayDType)) {
            తిరిగి arrayp->subDTypep()->వెడల్పు() > 1;
        }
        స్థిరరాశి AstStructDType* స్థిరరాశి structp = VN_CAST(dtypep, StructDType);
        తిరిగి structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    స్థిర తార్కిక isSplittableType(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        ఒకవేళ (isPacked(dtypep)) తిరిగి v3Global.opt.fDecomposePacked();
        ఒకవేళ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ఒకవేళ (స్థిరరాశి AstStructDType* స్థిరరాశి structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                కొరకు (స్థిరరాశి AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ఒకవేళ (mp->valuep()) తిరిగి అబద్ధం;
                }
            }
            తిరిగి v3Global.opt.fDecomposeUnpacked();
        }
        తిరిగి అబద్ధం;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    స్థిర Ast_కణుపు_DType* dtype_యొక్క(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) తిరిగి refp->varp()->dtypep();
        తిరిగి nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    స్థిరరాశి std::సదిశ<Component>& dtypeComponents(Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::సదిశ<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ఒకవేళ (compsr.ఖాళీలు()) {
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి arrayp = VN_CAST(dtypep, కణుపు_Array_DType)) {
                స్థిరరాశి తార్కిక packed = isPacked(dtypep);
                స్థిరరాశి VNumRange range = arrayp->declRange();
                స్థిరరాశి తార్కిక rev = packed && range.ascending();  // Only for naming components
                Ast_కణుపు_DType* స్థిరరాశి subp = arrayp->subDTypep();
                కొరకు (పూర్ణాంకం i = 0; i < range.elements(); ++i) {
                    స్థిరరాశి పూర్ణాంకం lsb = packed ? i * subp->వెడల్పు() : 0;
                    స్థిరరాశి పూర్ణాంకం msb = packed ? lsb + subp->వెడల్పు() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    స్థిరరాశి పూర్ణాంకం declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.నెట్టు_వెనుక({subp, శూన్యసూచిక, declIdx, lsb, msb});
                }
            } లేకపోతే {
                స్థిరరాశి AstStructDType* స్థిరరాశి structp = VN_AS(dtypep, StructDType);
                స్థిరరాశి తార్కిక packed = isPacked(dtypep);
                కొరకు (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    స్థిరరాశి పూర్ణాంకం lsb = packed ? mp->lsb() : 0;
                    స్థిరరాశి పూర్ణాంకం msb = packed ? lsb + mp->వెడల్పు() - 1 : 0;
                    compsr.నెట్టు_వెనుక({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::తిరగేయి(compsr.begin(), compsr.చివర());
            }
        }

        // The components of this data type
        తిరిగి compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    కొలత_t component_సూచీ(Ast_కణుపు_DType* dtypep, పూర్ణాంకం బిట్టు_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        స్థిరరాశి std::సదిశ<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        స్థిరరాశి స్వయంచాలక it = std::క్రింది_bound(compsr.begin(), compsr.చివర(), బిట్టు_2,
                                         [](స్థిరరాశి Component& comp, పూర్ణాంకం b) {  //
                                             తిరిగి b > comp.msb;
                                         });
        తిరిగి it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    ఉంచు* component_యొక్క(ఉంచు* placep, కొలత_t idx) {
        స్థిరరాశి std::సదిశ<Component>& comps = dtypeComponents(placep->dtypep);
        ఒకవేళ (placep->childrenp.ఖాళీలు()) placep->childrenp.resize(comps.కొలత());
        std::unique_ptr<ఉంచు>& childpr = placep->childrenp.at(idx);
        ఒకవేళ (!childpr) {
            childpr = std::make_unique<ఉంచు>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->కోరుతుంది_విభజించు = placep->rootp->vscp->varp()->attr_విభజించు_Var();
        }
        తిరిగి childpr.get();
    }

    // Is 'nodep' cheap to clone
    స్థిర తార్కిక isCheap(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        // Constants are cheap
        ఒకవేళ (VN_IS(nodep, Const)) తిరిగి నిజం;
        // So are resets
        ఒకవేళ (VN_IS(nodep, CReset)) తిరిగి నిజం;
        // Variable references are cheap for the most part
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ఒకవేళ (refp->varp()->isForced()) తిరిగి అబద్ధం;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ఒకవేళ (refp->varp()->isSc()) తిరిగి అబద్ధం;
            // Otherwise the path rooted here is cheap
            తిరిగి నిజం;
        }
        // Bit selects only with a constant LSB
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            తిరిగి VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి selp = VN_CAST(nodep, ArraySel)) {
            తిరిగి isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {  //
            తిరిగి isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ఒకవేళ (స్థిరరాశి AstExtend* స్థిరరాశి extp = VN_CAST(nodep, Extend)) {  //
            తిరిగి isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        తిరిగి అబద్ధం;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

తరగతి Decompose_రికార్డు final : బహిరంగ VNVisitorConst, బహిరంగ Decompose_పునాది {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_కణుపు_కేటాయించు* m_assignp = శూన్యసూచిక;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    స్థిర శూన్య warn_No_విభజించు(AstVar* varp, స్థిరరాశి Ast_కణుపు* wherep, స్థిరరాశి అక్షరం* reasonp) {
        // Only warn if user requested splitting
        ఒకవేళ (!varp->attr_విభజించు_Var()) తిరిగి;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ఒకవేళ (isPacked(varp->dtypep())) తిరిగి;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_సంకేతం::SPLITVAR, నిజం);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    స్థిర తార్కిక isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ఒకవేళ (!varp->user1()) {
            స్థిరరాశి తార్కిక eligible = [&]() {
                // Check data type
                ఒకవేళ (!isSplittableType(varp->dtypep())) తిరిగి అబద్ధం;
                // Check properties
                స్థిరరాశి అక్షరం* స్థిరరాశి reasonp = varp->cannot_విభజించు_రకం_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ఒకవేళ (reasonp) warn_No_విభజించు(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                తిరిగి !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        తిరిగి varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    ఉంచు* ఉంచు_యొక్క(స్థిరరాశి AstVarRef* refp) {
        AstVarScope* స్థిరరాశి vscp = refp->varScopep();
        ఒకవేళ (!isEligible(vscp->varp())) తిరిగి శూన్యసూచిక;
        ఉంచు& ఉంచు_2 = m_state.places(vscp);
        ఒకవేళ (!ఉంచు_2.rootp) {
            ఉంచు_2.rootp = &ఉంచు_2;
            ఉంచు_2.dtypep = vscp->varp()->dtypep();
            ఉంచు_2.vscp = vscp;
            ఉంచు_2.కోరుతుంది_విభజించు = vscp->varp()->attr_విభజించు_Var();
            m_state.rootps.నెట్టు_వెనుక(&ఉంచు_2);
        }
        తిరిగి &ఉంచు_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    స్థిర శూన్య ఖండం(ఉంచు* placep, స్థిరరాశి Ast_కణుపు* wherep) {
        placep->splittable = అబద్ధం;
        // Warn on the variable only, a component is split as far as possible, as requested
        ఒకవేళ (!placep->parentp) {
            warn_No_విభజించు(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    ఉంచు* resolveChain(Ast_కణుపు_సమాసం* exprp, std::సదిశ<Select>& chain) {
        // Array selects correspond one to one with a component select
        ఒకవేళ (AstArraySel* స్థిరరాశి selp = VN_CAST(exprp, ArraySel)) {
            ఒకవేళ (ఉంచు* స్థిరరాశి fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                స్థిరరాశి AstConst* స్థిరరాశి bitp = VN_CAST(selp->bitp(), Const);
                ఒకవేళ (bitp && bitp->గమ్యం_UQuad() < dtypeComponents(fromp->dtypep).కొలత()) {
                    ఉంచు* స్థిరరాశి childp = component_యొక్క(fromp, bitp->గమ్యం_UQuad());
                    fromp->కోరుతుంది_విభజించు = నిజం;
                    chain.నెట్టు_వెనుక({selp, childp, 0});
                    తిరిగి childp;
                }
                // Otherwise block splitting of the Place
                ఖండం(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            తిరిగి శూన్యసూచిక;
        }
        // Struct selects correspond one to one with a component select
        ఒకవేళ (AstStructSel* స్థిరరాశి selp = VN_CAST(exprp, StructSel)) {
            ఒకవేళ (ఉంచు* స్థిరరాశి fromp = resolveChain(selp->fromp(), chain)) {
                ఒకవేళ (!isUnpacked(fromp->dtypep)) తిరిగి శూన్యసూచిక;
                AstStructDType* స్థిరరాశి structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ఒకవేళ (!structp->user1_సమితి_Once()) {
                    స్థిరరాశి std::సదిశ<Component>& compsr = dtypeComponents(structp);
                    కొరకు (కొలత_t i = 0; i < compsr.కొలత(); ++i) compsr[i].memberp->user1(i);
                }
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి memberp = m_memberMap.కనుగొను_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                ఉంచు* స్థిరరాశి childp = component_యొక్క(fromp, memberp->user1());
                fromp->కోరుతుంది_విభజించు = నిజం;
                chain.నెట్టు_వెనుక({selp, childp, 0});
                తిరిగి childp;
            }
            తిరిగి శూన్యసూచిక;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(exprp, Sel)) {
            ఉంచు* placep = resolveChain(selp->fromp(), chain);
            ఒకవేళ (!placep) {
                iterateConst(selp->lsbp());
                తిరిగి శూన్యసూచిక;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(selp->lsbp(), Const);
            ఒకవేళ (!lsbp || lsbp->గమ్యం_SInt() + selp->వెడల్పు_Const() > placep->dtypep->వెడల్పు()) {
                ఖండం(placep, selp->fromp());
                iterateConst(selp->lsbp());
                తిరిగి శూన్యసూచిక;
            }
            // Descend into the Place containing the bits, until exact
            పూర్ణాంకం lsb = lsbp->గమ్యం_SInt();
            పూర్ణాంకం msb = lsb + selp->వెడల్పు_Const() - 1;
            అయితే (lsb != 0 || msb != placep->dtypep->వెడల్పు() - 1) {
                // Within a component of a type not split
                ఒకవేళ (!isPacked(placep->dtypep)) తిరిగి శూన్యసూచిక;
                స్థిరరాశి కొలత_t idx = component_సూచీ(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ఒకవేళ (idx != component_సూచీ(placep->dtypep, msb)) {
                    ఖండం(placep, selp->fromp());
                    తిరిగి శూన్యసూచిక;
                }
                placep->కోరుతుంది_విభజించు = నిజం;
                స్థిరరాశి Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_యొక్క(placep, idx);
                chain.నెట్టు_వెనుక({selp, placep, lsb});
            }
            తిరిగి placep;
        }
        // Base case
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) తిరిగి ఉంచు_యొక్క(refp);
        // Not a select chain
        iterateConst(exprp);
        తిరిగి శూన్యసూచిక;
    }

    // 'nodep' references the Place 'placep' whole
    శూన్య referenced_సంపూర్ణ(Ast_కణుపు_సమాసం* nodep, ఉంచు* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ఒకవేళ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            తిరిగి;
        }

        // Otherwise must block splitting of this Place
        ఖండం(placep, nodep);
    }

    // Visit a select expression
    శూన్య సందర్శించు_Select(Ast_కణుపు_సమాసం* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::సదిశ<Select> chain;
        ఉంచు* స్థిరరాశి placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ఒకవేళ (placep) referenced_సంపూర్ణ(nodep, placep);
        // Record the chain for the rewriting phase
        ఒకవేళ (!chain.ఖాళీలు()) m_state.chains.నెట్టు_వెనుక(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    స్థిర తార్కిక hasReversedRange(స్థిరరాశి Ast_కణుపు_DType* aDTypep, స్థిరరాశి Ast_కణుపు_DType* bDTypep) {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ఒకవేళ (!aArrayp || !bArrayp) తిరిగి అబద్ధం;
        ఒకవేళ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) తిరిగి నిజం;
        తిరిగి hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    శూన్య సందర్శించు_Assignment(Ast_కణుపు_కేటాయించు* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        స్థిరరాశి తార్కిక reversed = hasReversedRange(dtype_యొక్క(nodep->lhsp()), dtype_యొక్క(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? శూన్యసూచిక : nodep;
        iterateChildrenConst(nodep);
        ఒకవేళ (!m_assignp) తిరిగి;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
        ఉంచు* స్థిరరాశి lPlacep = lhsp->user1u().గమ్యం<ఉంచు*>();
        ఉంచు* స్థిరరాశి rPlacep = rhsp->user1u().గమ్యం<ఉంచు*>();

        // Is it a splittable assignment
        స్థిరరాశి తార్కిక splittable = [&]() {
            // Assignment between two Places is splittable
            ఒకవేళ (lPlacep && rPlacep) తిరిగి నిజం;
            // If the LHS is a Place, the RHS must be a splittable expression
            ఒకవేళ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ఒకవేళ (isPacked(lPlacep->dtypep)) తిరిగి నిజం;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                తిరిగి isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ఒకవేళ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ఒకవేళ (isPacked(rPlacep->dtypep)) తిరిగి నిజం;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                తిరిగి isCheap(lhsp);
            }
            // Otherwise not splittable
            తిరిగి అబద్ధం;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ఒకవేళ (!splittable) {
            ఒకవేళ (rPlacep) ఖండం(rPlacep, rhsp);
            ఒకవేళ (lPlacep) ఖండం(lPlacep, lhsp);
            తిరిగి;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ఒకవేళ (lPlacep && rPlacep) {
            స్థిరరాశి పూర్ణాంకం వెడల్పు = lPlacep->dtypep->వెడల్పు();  // Unused when unpacked
            lPlacep->copies.నెట్టు_వెనుక({rPlacep, 0, 0, వెడల్పు});
            rPlacep->copies.నెట్టు_వెనుక({lPlacep, 0, 0, వెడల్పు});
        }
        // Record for the rewriting phase
        m_state.assignments.నెట్టు_వెనుక({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_DType*) override {}  // No references in data types
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    శూన్య సందర్శించు(AstArraySel* nodep) override { సందర్శించు_Select(nodep); }
    శూన్య సందర్శించు(AstStructSel* nodep) override { సందర్శించు_Select(nodep); }
    శూన్య సందర్శించు(AstSel* nodep) override { సందర్శించు_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override { సందర్శించు_Assignment(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override { సందర్శించు_Assignment(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override { సందర్శించు_Assignment(nodep); }
    // VISITORS - References to Places
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ఒకవేళ (ఉంచు* స్థిరరాశి placep = ఉంచు_యొక్క(nodep)) referenced_సంపూర్ణ(nodep, placep);
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!isEligible(varp)) తిరిగి;
        varp->user1(2);  // Mark not eligible
        warn_No_విభజించు(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_రికార్డు(State& state, AstNetlist* netlistp)
        : Decompose_పునాది{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        కొరకు (ఉంచు* స్థిరరాశి placep : m_state.rootps) {
            ఒకవేళ (!isEligible(placep->vscp->varp())) placep->splittable = అబద్ధం;
        }
    }

బహిరంగ:
    స్థిర శూన్య apply(State& state, AstNetlist* netlistp) { Decompose_రికార్డు{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

తరగతి DecomposeDecision final : బహిరంగ Decompose_పునాది {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    std::సదిశ<ఉంచు*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::సదిశ<AstVar*>> m_విభజించు_Varps;
    VDouble0 m_stat_విభజించు_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_విభజించు_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_విభజించు_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_విభజించు_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    స్థిర తార్కిక can_విభజించు(స్థిరరాశి ఉంచు* placep) {
        // Not if not splittable
        ఒకవేళ (!placep->splittable) తిరిగి అబద్ధం;
        // Not if already split
        ఒకవేళ (placep->విభజించు) తిరిగి అబద్ధం;
        // Original variable or its parent was already split
        తిరిగి !placep->parentp || placep->parentp->విభజించు;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    శూన్య వరుసలో_చేర్చు(ఉంచు* placep) {
        ఒకవేళ (placep->queued || !placep->కోరుతుంది_విభజించు || !can_విభజించు(placep)) తిరిగి;
        placep->queued = నిజం;
        m_worklist.నెట్టు_వెనుక(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    శూన్య createComponents(ఉంచు* placep) {
        AstVarScope* స్థిరరాశి vscp = placep->vscp;
        AstVar* స్థిరరాశి varp = vscp->varp();
        std::సదిశ<AstVar*>& varps = m_విభజించు_Varps(varp);
        // Create the component AstVars on first encounter
        ఒకవేళ (varps.ఖాళీలు()) {
            స్థిరరాశి తార్కిక packed = isPacked(varp->dtypep());
            ఒకవేళ (placep->parentp) {
                ++(packed ? m_stat_విభజించు_Packed_Comp : m_stat_విభజించు_Unpacked_Comp);
            } లేకపోతే {
                ++(packed ? m_stat_విభజించు_Packed_Orig : m_stat_విభజించు_Unpacked_Orig);
            }
            File_పంక్తి* స్థిరరాశి flp = varp->fileline();
            స్థిరరాశి VVarType varType = varp->varType();
            స్థిరరాశి std::string name = varp->name();
            కొరకు (స్థిరరాశి Component& comp : dtypeComponents(varp->dtypep())) {
                స్థిరరాశి std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_కణుపు::encode_సంఖ్య(comp.declIdx) + "__KET__";
                AstVar* స్థిరరాశి newp = కొత్త AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_విభజించు_Attr_From(varp);
                varps.నెట్టు_వెనుక(newp);
                varp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        కొరకు (కొలత_t i = 0; i < varps.కొలత(); ++i) {
            AstVarScope* స్థిరరాశి newp = కొత్త AstVarScope{flp, scopep, varps[i]};
            component_యొక్క(placep, i)->vscp = newp;
            vscp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    శూన్య కూడు_Copy(ఉంచు* placep, స్థిరరాశి Copy& copy) {
        UASSERT(!placep->విభజించు, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ఒకవేళ (!placep->splittable) తిరిగి;
        // Add the copy to the Place
        placep->copies.నెట్టు_వెనుక(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ఒకవేళ (!isPacked(placep->dtypep)) తిరిగి;
        ఒకవేళ (copy.lsb != 0 || copy.వెడల్పు != placep->dtypep->వెడల్పు()) {
            placep->కోరుతుంది_విభజించు = నిజం;
            వరుసలో_చేర్చు(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    శూన్య newCopy(ఉంచు* ap, పూర్ణాంకం aLsb, ఉంచు* bp, పూర్ణాంకం bLsb, పూర్ణాంకం వెడల్పు) {
        UASSERT(ap != bp, "Copy within a Place");
        కూడు_Copy(ap, {bp, aLsb, bLsb, వెడల్పు});
        కూడు_Copy(bp, {ap, bLsb, aLsb, వెడల్పు});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    శూన్య విభజించు_Copy(ఉంచు* placep, స్థిరరాశి Copy& copy) {
        ఉంచు* స్థిరరాశి otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ఒకవేళ (isPacked(placep->dtypep)) {
            స్థిరరాశి std::సదిశ<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            స్థిరరాశి పూర్ణాంకం lsb = copy.lsb;
            స్థిరరాశి పూర్ణాంకం msb = copy.lsb + copy.వెడల్పు - 1;
            స్థిరరాశి కొలత_t మొదటి = component_సూచీ(placep->dtypep, lsb);
            కొరకు (కొలత_t i = మొదటి; i < comps.కొలత() && comps[i].lsb <= msb; ++i) {
                స్థిరరాశి Component& comp = comps[i];
                స్థిరరాశి పూర్ణాంకం partLsb = std::max(lsb, comp.lsb);
                స్థిరరాశి పూర్ణాంకం partMsb = std::min(msb, comp.msb);
                newCopy(component_యొక్క(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            తిరిగి;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->కోరుతుంది_విభజించు = నిజం;
        వరుసలో_చేర్చు(otherp);
        // The sides are the same shape, so pairwise whole copies
        స్థిరరాశి కొలత_t కొలత = dtypeComponents(placep->dtypep).కొలత();
        కొరకు (కొలత_t i = 0; i < కొలత; ++i) {
            ఉంచు* స్థిరరాశి ap = component_యొక్క(placep, i);
            ఒకవేళ (!ap->splittable) కొనసాగు;
            ఉంచు* స్థిరరాశి bp = component_యొక్క(otherp, i);
            ఒకవేళ (!bp->splittable) కొనసాగు;
            newCopy(ap, 0, bp, 0, ap->dtypep->వెడల్పు());
        }
    }

    // Split the Place
    శూన్య విభజించు_ఉంచు(ఉంచు* placep) {
        UASSERT(!placep->విభజించు, "Place split twice");
        // It is now split
        placep->విభజించు = నిజం;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        స్థిరరాశి కొలత_t కొలత = dtypeComponents(placep->dtypep).కొలత();
        కొరకు (కొలత_t i = 0; i < కొలత; ++i) వరుసలో_చేర్చు(component_యొక్క(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        కొరకు (స్థిరరాశి Copy& copy : placep->copies) {
            ఒకవేళ (!copy.otherp->విభజించు) విభజించు_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    స్పష్టమైన DecomposeDecision(State& state)
        : Decompose_పునాది{state} {
        // Enqueue the original variables
        కొరకు (ఉంచు* స్థిరరాశి placep : m_state.rootps) వరుసలో_చేర్చు(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        అయితే (!m_worklist.ఖాళీలు()) {
            ఉంచు* స్థిరరాశి placep = m_worklist.వెనుక();
            m_worklist.తీయి_వెనుక();
            placep->queued = అబద్ధం;
            విభజించు_ఉంచు(placep);
        }

        V3Stats::కూడు_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_విభజించు_Unpacked_Orig);
        V3Stats::కూడు_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_విభజించు_Packed_Orig);
        V3Stats::కూడు_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_విభజించు_Unpacked_Comp);
        V3Stats::కూడు_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_విభజించు_Packed_Comp);
    }

బహిరంగ:
    స్థిర శూన్య apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

తరగతి DecomposeRewrite final : బహిరంగ VNDeleter, బహిరంగ Decompose_పునాది {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    స్థిర AstVarRef* newRef(File_పంక్తి* flp, స్థిరరాశి ఉంచు* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        తిరిగి కొత్త AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    శూన్య rewriteChain(స్థిరరాశి std::సదిశ<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        స్థిరరాశి Select* selectp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి Select& select : chain) {
            ఒకవేళ (select.placep->parentp->విభజించు) selectp = &select;
        }
        ఒకవేళ (!selectp) తిరిగి;
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        స్థిరరాశి VAccess access = [&]() {
            స్థిరరాశి Ast_కణుపు_సమాసం* rootp = chain.front().exprp;
            అయితే (నిజం) {
                ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } లేకపోతే {
                    తిరిగి VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        Ast_కణుపు_సమాసం* newp = newRef(flp, selectp->placep, access);
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(exprp, Sel)) {
            స్థిరరాశి పూర్ణాంకం వెడల్పు = selp->వెడల్పు_Const();
            ఒకవేళ (వెడల్పు != newp->వెడల్పు()) newp = కొత్త AstSel{flp, newp, selectp->lsb, వెడల్పు};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_కణుపు_సమాసం* newSelect(Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtypep, కొలత_t idx) {
        File_పంక్తి* స్థిరరాశి flp = fromp->fileline();
        స్థిరరాశి Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ఒకవేళ (isPacked(dtypep)) {
            AstSel* స్థిరరాశి selp = కొత్త AstSel{flp, fromp, comp.lsb, comp.dtypep->వెడల్పు()};
            selp->dtypep(comp.dtypep);
            తిరిగి selp;
        }
        // If unpacked array, it's an ArraySel
        ఒకవేళ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            స్థిరరాశి పూర్ణాంకం i = static_cast<పూర్ణాంకం>(idx);
            UASSERT_OBJ(static_cast<కొలత_t>(i) == idx, fromp, "Index overflow");
            తిరిగి కొత్త AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* స్థిరరాశి selp = కొత్త AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        తిరిగి selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    స్థిర std::సదిశ<Ast_కణుపు_సమాసం*> concatTerms(Ast_కణుపు_సమాసం* nodep) {
        std::సదిశ<Ast_కణుపు_సమాసం*> termps;
        std::సదిశ<Ast_కణుపు_సమాసం*> దొంతర{nodep};
        అయితే (!దొంతర.ఖాళీలు()) {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = దొంతర.వెనుక();
            దొంతర.తీయి_వెనుక();
            ఒకవేళ (AstConcat* స్థిరరాశి concatp = VN_CAST(exprp, Concat)) {
                దొంతర.నెట్టు_వెనుక(concatp->lhsp());
                దొంతర.నెట్టు_వెనుక(concatp->rhsp());
                కొనసాగు;
            }
            ఒకవేళ (AstReplicate* స్థిరరాశి repp = VN_CAST(exprp, Replicate)) {
                స్థిరరాశి AstConst* స్థిరరాశి countp = VN_AS(repp->countp(), Const);
                కొరకు (uint32_t i = 0; i < countp->గమ్యం_UInt(); ++i) దొంతర.నెట్టు_వెనుక(repp->srcp());
                కొనసాగు;
            }
            termps.నెట్టు_వెనుక(exprp);
        }
        తిరిగి termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    స్థిర తార్కిక isSliceable(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            తిరిగి VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ఒకవేళ (స్థిరరాశి AstConcat* స్థిరరాశి catp = VN_CAST(nodep, Concat)) {
            తిరిగి isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ఒకవేళ (స్థిరరాశి AstCond* స్థిరరాశి condp = VN_CAST(nodep, Cond)) {
            తిరిగి isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(nodep, And)) {
            తిరిగి isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ఒకవేళ (స్థిరరాశి AstOr* స్థిరరాశి orp = VN_CAST(nodep, Or)) {
            తిరిగి isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ఒకవేళ (స్థిరరాశి AstXor* స్థిరరాశి xorp = VN_CAST(nodep, Xor)) {
            తిరిగి isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ఒకవేళ (స్థిరరాశి AstExtend* స్థిరరాశి extp = VN_CAST(nodep, Extend)) {
            తిరిగి isSliceable(extp->lhsp());
        }
        ఒకవేళ (స్థిరరాశి AstNot* స్థిరరాశి notp = VN_CAST(nodep, Not)) {  //
            తిరిగి isSliceable(notp->lhsp());
        }
        ఒకవేళ (స్థిరరాశి AstReplicate* స్థిరరాశి repp = VN_CAST(nodep, Replicate)) {
            తిరిగి isSliceable(repp->srcp());
        }
        తిరిగి isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    స్థిర Ast_కణుపు_సమాసం* new_ముక్క(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం lsb, పూర్ణాంకం వెడల్పు) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            V3_సంఖ్య అంకె{nodep, వెడల్పు};
            అంకె.opSel(constp->అంకె(), lsb + వెడల్పు - 1, lsb);
            తిరిగి కొత్త AstConst{flp, అంకె};
        }
        // A constant bit select, from the bits of what it selects from
        ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(selp->lsbp(), Const)) {
                తిరిగి new_ముక్క(selp->fromp(), lsbp->గమ్యం_SInt() + lsb, వెడల్పు);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ఒకవేళ (AstConcat* స్థిరరాశి catp = VN_CAST(nodep, Concat)) {
            స్థిరరాశి పూర్ణాంకం r_వెడల్పు = catp->rhsp()->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
            ఒకవేళ (msb < r_వెడల్పు) తిరిగి new_ముక్క(catp->rhsp(), lsb, వెడల్పు);
            ఒకవేళ (lsb >= r_వెడల్పు) తిరిగి new_ముక్క(catp->lhsp(), lsb - r_వెడల్పు, వెడల్పు);
            తిరిగి కొత్త AstConcat{flp, new_ముక్క(catp->lhsp(), 0, msb - r_వెడల్పు + 1),
                                 new_ముక్క(catp->rhsp(), lsb, r_వెడల్పు - lsb)};
        }
        ఒకవేళ (AstCond* స్థిరరాశి condp = VN_CAST(nodep, Cond)) {
            తిరిగి కొత్త AstCond{flp, condp->condp()->clone_వృక్షం_శుద్ధ(అబద్ధం),
                               new_ముక్క(condp->thenp(), lsb, వెడల్పు),
                               new_ముక్క(condp->elsep(), lsb, వెడల్పు)};
        }
        ఒకవేళ (AstAnd* స్థిరరాశి andp = VN_CAST(nodep, And)) {
            తిరిగి కొత్త AstAnd{flp, new_ముక్క(andp->lhsp(), lsb, వెడల్పు),
                              new_ముక్క(andp->rhsp(), lsb, వెడల్పు)};
        }
        ఒకవేళ (AstOr* స్థిరరాశి orp = VN_CAST(nodep, Or)) {
            తిరిగి కొత్త AstOr{flp, new_ముక్క(orp->lhsp(), lsb, వెడల్పు),
                             new_ముక్క(orp->rhsp(), lsb, వెడల్పు)};
        }
        ఒకవేళ (AstXor* స్థిరరాశి xorp = VN_CAST(nodep, Xor)) {
            తిరిగి కొత్త AstXor{flp, new_ముక్క(xorp->lhsp(), lsb, వెడల్పు),
                              new_ముక్క(xorp->rhsp(), lsb, వెడల్పు)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ఒకవేళ (AstExtend* స్థిరరాశి extp = VN_CAST(nodep, Extend)) {
            స్థిరరాశి పూర్ణాంకం src_వెడల్పు = extp->lhsp()->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
            ఒకవేళ (msb < src_వెడల్పు) తిరిగి new_ముక్క(extp->lhsp(), lsb, వెడల్పు);
            ఒకవేళ (lsb >= src_వెడల్పు) తిరిగి కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, వెడల్పు, 0};
            తిరిగి కొత్త AstConcat{
                flp, కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, msb - src_వెడల్పు + 1, 0},
                new_ముక్క(extp->lhsp(), lsb, src_వెడల్పు - lsb)};
        }
        ఒకవేళ (AstNot* స్థిరరాశి notp = VN_CAST(nodep, Not)) {
            తిరిగి కొత్త AstNot{flp, new_ముక్క(notp->lhsp(), lsb, వెడల్పు)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ఒకవేళ (AstReplicate* స్థిరరాశి repp = VN_CAST(nodep, Replicate)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి srcp = repp->srcp();
            స్థిరరాశి పూర్ణాంకం src_వెడల్పు = srcp->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
            Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
            కొరకు (పూర్ణాంకం copyLsb = lsb / src_వెడల్పు * src_వెడల్పు; copyLsb <= msb; copyLsb += src_వెడల్పు) {
                స్థిరరాశి పూర్ణాంకం partLsb = std::max(lsb, copyLsb) - copyLsb;
                స్థిరరాశి పూర్ణాంకం partMsb = std::min(msb, copyLsb + src_వెడల్పు - 1) - copyLsb;
                Ast_కణుపు_సమాసం* స్థిరరాశి bitsp = new_ముక్క(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? కొత్త AstConcat{flp, bitsp, resultp} : bitsp;
            }
            తిరిగి resultp;
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి clonep = nodep->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (lsb == 0 && వెడల్పు == nodep->వెడల్పు()) తిరిగి clonep;
        తిరిగి కొత్త AstSel{flp, clonep, lsb, వెడల్పు};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::సదిశ<Ast_కణుపు_సమాసం*> new_కేటాయించు_Rhsps(Ast_కణుపు_సమాసం* nodep, Ast_కణుపు_DType* dtypep) {
        స్థిరరాశి std::సదిశ<Component>& compsr = dtypeComponents(dtypep);
        std::సదిశ<Ast_కణుపు_సమాసం*> valueps;
        valueps.reserve(compsr.కొలత());

        // If CReset, it is duplicated for each component
        ఒకవేళ (AstCReset* స్థిరరాశి cresetp = VN_CAST(nodep, CReset)) {
            కొరకు (స్థిరరాశి Component& comp : compsr) {
                AstCReset* స్థిరరాశి newp = cresetp->clone_వృక్షం(అబద్ధం);
                newp->dtypep(comp.dtypep);
                valueps.నెట్టు_వెనుక(newp);
            }
            తిరిగి valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ఒకవేళ (isPacked(dtypep)) {
            valueps.resize(compsr.కొలత(), శూన్యసూచిక);
            పూర్ణాంకం lsb = 0;  // LSB of the current term
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి termp : concatTerms(nodep)) {
                స్థిరరాశి పూర్ణాంకం msb = lsb + termp->వెడల్పు() - 1;
                స్థిరరాశి కొలత_t మొదటి = component_సూచీ(dtypep, lsb);
                కొరకు (కొలత_t i = మొదటి; i < compsr.కొలత() && compsr[i].lsb <= msb; ++i) {
                    స్థిరరాశి Component& comp = compsr[i];
                    స్థిరరాశి పూర్ణాంకం partLsb = std::max(lsb, comp.lsb);
                    స్థిరరాశి పూర్ణాంకం partMsb = std::min(msb, comp.msb);
                    File_పంక్తి* స్థిరరాశి flp = termp->fileline();
                    Ast_కణుపు_సమాసం* స్థిరరాశి sp = new_ముక్క(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? కొత్త AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            తిరిగి valueps;
        }

        // Otherwise unpacked, so select each component
        కొరకు (కొలత_t i = 0; i < compsr.కొలత(); ++i) {
            valueps.నెట్టు_వెనుక(newSelect(nodep->clone_వృక్షం(అబద్ధం), dtypep, i));
        }
        తిరిగి valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_కణుపు_సమాసం* new_బిట్లు(File_పంక్తి* flp, స్థిరరాశి ఉంచు* placep, పూర్ణాంకం lsb, పూర్ణాంకం msb) {
        Ast_కణుపు_DType* స్థిరరాశి dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->విభజించు, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ఒకవేళ (!placep->విభజించు) {
            AstVarRef* స్థిరరాశి refp = newRef(flp, placep, VAccess::READ);
            ఒకవేళ (lsb == 0 && msb == dtypep->వెడల్పు() - 1) తిరిగి refp;
            తిరిగి కొత్త AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        స్థిరరాశి std::సదిశ<Component>& comps = dtypeComponents(dtypep);
        Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
        స్థిరరాశి కొలత_t మొదటి = component_సూచీ(dtypep, lsb);
        కొరకు (కొలత_t i = మొదటి; i < comps.కొలత() && comps[i].lsb <= msb; ++i) {
            స్థిరరాశి Component& comp = comps[i];
            స్థిరరాశి పూర్ణాంకం partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            స్థిరరాశి పూర్ణాంకం partMsb = std::min(msb, comp.msb) - comp.lsb;
            ఉంచు* స్థిరరాశి childp = placep->childrenp.at(i).get();
            Ast_కణుపు_సమాసం* స్థిరరాశి bitsp = new_బిట్లు(flp, childp, partLsb, partMsb);
            resultp = resultp ? కొత్త AstConcat{flp, bitsp, resultp} : bitsp;
        }
        తిరిగి resultp;
    }

    స్థిర శూన్య కూడు_New_కేటాయించు(Ast_కణుపు_కేటాయించు* origp, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) {
        origp->కూడు_ఇక్కడ_This_As_తదుపరి(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    శూన్య expandSS(Ast_కణుపు_కేటాయించు* origp, ఉంచు* lPlacep, ఉంచు* rPlacep) {
        File_పంక్తి* స్థిరరాశి flp = origp->fileline();
        స్థిరరాశి std::సదిశ<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ఒకవేళ (isPacked(rPlacep->dtypep)) {
            కొరకు (కొలత_t i = 0; i < compsr.కొలత(); ++i) {
                ఉంచు* స్థిరరాశి lChildp = lPlacep->childrenp.at(i).get();
                Ast_కణుపు_సమాసం* స్థిరరాశి rp = new_బిట్లు(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, శూన్యసూచిక, శూన్యసూచిక, rp);
            }
        } లేకపోతే {
            కొరకు (కొలత_t i = 0; i < compsr.కొలత(); ++i) {
                ఉంచు* స్థిరరాశి lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, శూన్యసూచిక, rPlacep->childrenp.at(i).get(), శూన్యసూచిక);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    శూన్య expandSN(Ast_కణుపు_కేటాయించు* origp, ఉంచు* lPlacep, Ast_కణుపు_సమాసం* rhsp) {
        స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*> valueps = new_కేటాయించు_Rhsps(rhsp, lPlacep->dtypep);
        కొరకు (కొలత_t i = 0; i < valueps.కొలత(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), శూన్యసూచిక, శూన్యసూచిక, valueps[i]);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    శూన్య expandNS(Ast_కణుపు_కేటాయించు* origp, Ast_కణుపు_సమాసం* lhsp, ఉంచు* rPlacep) {
        File_పంక్తి* స్థిరరాశి flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ఒకవేళ (isPacked(rPlacep->dtypep)) {
            కూడు_New_కేటాయించు(origp, lhsp, new_బిట్లు(flp, rPlacep, 0, rPlacep->dtypep->వెడల్పు() - 1));
            తిరిగి;
        }
        // Unpacked of the same shape, component-wise
        Ast_కణుపు_DType* స్థిరరాశి dtypep = dtype_యొక్క(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        స్థిరరాశి తార్కిక readsWrittenVar = !VN_IS(origp, కేటాయించు_Dly) && [&]() {
            స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
            తిరిగి lhsp->exists([](స్థిరరాశి AstVarRef* refp) {
                AstVarScope* స్థిరరాశి vscp = refp->varScopep();
                ఒకవేళ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ఒకవేళ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                తిరిగి vscp->user2() == 3;
            });
        }();
        ఒకవేళ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* స్థిరరాశి scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* స్థిరరాశి vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, శూన్యసూచిక, కొత్త AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, శూన్యసూచిక);
            కూడు_New_కేటాయించు(origp, lhsp, కొత్త AstVarRef{flp, vscp, VAccess::READ});
            తిరిగి;
        }
        స్థిరరాశి కొలత_t కొలత = dtypeComponents(dtypep).కొలత();
        కొరకు (కొలత_t i = 0; i < కొలత; ++i) {
            Ast_కణుపు_సమాసం* స్థిరరాశి newLhsp = newSelect(lhsp->clone_వృక్షం(అబద్ధం), dtypep, i);
            expand(origp, శూన్యసూచిక, newLhsp, rPlacep->childrenp.at(i).get(), శూన్యసూచిక);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    శూన్య expand(Ast_కణుపు_కేటాయించు* origp,  //
                ఉంచు* lPlacep, Ast_కణుపు_సమాసం* lhsp,  //
                ఉంచు* rPlacep, Ast_కణుపు_సమాసం* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_పంక్తి* స్థిరరాశి flp = origp->fileline();
        // A Place not split is referenced
        ఒకవేళ (lPlacep && !lPlacep->విభజించు) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = శూన్యసూచిక;
        }
        ఒకవేళ (rPlacep && !rPlacep->విభజించు) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = శూన్యసూచిక;
        }
        ఒకవేళ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } లేకపోతే ఒకవేళ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } లేకపోతే ఒకవేళ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } లేకపోతే {
            కూడు_New_కేటాయించు(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    తార్కిక is_విభజించు_Between(స్థిరరాశి ఉంచు* placep, పూర్ణాంకం lsb, పూర్ణాంకం msb) {
        అయితే (placep->విభజించు) {
            స్థిరరాశి కొలత_t idx = component_సూచీ(placep->dtypep, lsb);
            ఒకవేళ (idx != component_సూచీ(placep->dtypep, msb)) తిరిగి నిజం;
            స్థిరరాశి Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        తిరిగి అబద్ధం;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    శూన్య hoistTerms(Ast_కణుపు_కేటాయించు* origp, స్థిరరాశి ఉంచు* lPlacep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = origp->rhsp();
        స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* స్థిరరాశి scopep = lPlacep->rootp->vscp->scopep();
        పూర్ణాంకం lsb = 0;  // LSB of the current term
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి termp : termps) {
            స్థిరరాశి పూర్ణాంకం tLsb = lsb;
            స్థిరరాశి పూర్ణాంకం tMsb = lsb + termp->వెడల్పు() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ఒకవేళ (!termp->user2()) కొనసాగు;
            // Cheap to select from
            ఒకవేళ (isCheap(termp)) {
                termp->user2(0);
                కొనసాగు;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ఒకవేళ (termp->is_శుద్ధ() && termp->user2() == 1 && !is_విభజించు_Between(lPlacep, tLsb, tMsb)) {
                కొనసాగు;
            }
            // Bits can be selected without evaluating it more than once
            ఒకవేళ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                కొనసాగు;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_పంక్తి* స్థిరరాశి flp = termp->fileline();
            AstVarScope* స్థిరరాశి vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(కొత్త AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* స్థిరరాశి tmpRefp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
            origp->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త Ast_కేటాయించు{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    స్థిర శూన్య markComponents(స్థిరరాశి ఉంచు* placep) {
        కొరకు (స్థిరరాశి std::unique_ptr<ఉంచు>& childp : placep->childrenp) {
            childp->vscp->user2(నిజం);
            ఒకవేళ (childp->విభజించు) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    శూన్య rewriteAssignment(స్థిరరాశి Assignment& assignment) {
        ఉంచు* స్థిరరాశి lPlacep = assignment.lPlacep;
        ఉంచు* స్థిరరాశి rPlacep = assignment.rPlacep;
        స్థిరరాశి తార్కిక l_విభజించు = lPlacep && lPlacep->విభజించు;
        స్థిరరాశి తార్కిక r_విభజించు = rPlacep && rPlacep->విభజించు;
        // Nothing to do if neither side is split
        ఒకవేళ (!l_విభజించు && !r_విభజించు) తిరిగి;

        Ast_కణుపు_కేటాయించు* స్థిరరాశి origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ఒకవేళ (l_విభజించు && !r_విభజించు && !VN_IS(origp, కేటాయించు_Dly)) {
            స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
            markComponents(lPlacep);
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = origp->rhsp();
            ఒకవేళ (rhsp->exists([](స్థిరరాశి AstVarRef* refp) { తిరిగి refp->varScopep()->user2(); })) {
                File_పంక్తి* స్థిరరాశి flp = rhsp->fileline();
                AstScope* స్థిరరాశి scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* స్థిరరాశి vscp = m_tmps.make(flp, scopep, dtype_యొక్క(rhsp));
                rhsp->replaceWith(కొత్త AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* స్థిరరాశి tmpRefp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
                origp->కూడు_ఇక్కడ_This_As_తదుపరి(కొత్త Ast_కేటాయించు{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ఒకవేళ (l_విభజించు && !r_విభజించు && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = origp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = origp->rhsp()->unlink_Fr_వెనుక();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_విభజించు ? lPlacep : శూన్యసూచిక, l_విభజించు ? శూన్యసూచిక : lhsp,  //
               r_విభజించు ? rPlacep : శూన్యసూచిక, r_విభజించు ? శూన్యసూచిక : rhsp);
        ఒకవేళ (l_విభజించు) VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
        ఒకవేళ (r_విభజించు) VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(నెట్టు_Deletep(origp->unlink_Fr_వెనుక()), origp);
    }

    // The combinational AstActive of the scope, cached
    స్థిర Ast_చురుకైన* combo_చురుకైన(AstScope* scopep) {
        ఒకవేళ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(నిజం));
        తిరిగి VN_AS(scopep->user1p(), చురుకైన);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    శూన్య driveFromComponents(ఉంచు* placep, Ast_చురుకైన* activep) {
        ఒకవేళ (!placep->విభజించు) తిరిగి;
        AstVarScope* స్థిరరాశి vscp = placep->vscp;
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        కొరకు (కొలత_t i = 0; i < placep->childrenp.కొలత(); ++i) {
            ఉంచు* స్థిరరాశి childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsRefp = కొత్త AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->కూడు_Stmtsp(కొత్త AstAlways{కొత్త Ast_కేటాయించు_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    స్పష్టమైన DecomposeRewrite(State& state)
        : Decompose_పునాది{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        కొరకు (స్థిరరాశి std::సదిశ<Select>& chain : m_state.chains) rewriteChain(chain);
        కొరకు (స్థిరరాశి Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        కొరకు (ఉంచు* స్థిరరాశి placep : m_state.rootps) {
            ఒకవేళ (!placep->విభజించు) కొనసాగు;
            స్థిరరాశి AstVarScope* స్థిరరాశి vscp = placep->vscp;
            // Only traced ones at this point
            ఒకవేళ (!(vscp->varp()->isTrace() && vscp->isTrace())) కొనసాగు;
            Ast_చురుకైన* స్థిరరాశి activep = combo_చురుకైన(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::కూడు_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::కూడు_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::కూడు_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::కూడు_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

బహిరంగ:
    స్థిర శూన్య apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

శూన్య V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_పునాది::State state;
        Decompose_రికార్డు::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("decompose", 0, dump_వృక్షం_Either_Level() >= 3);
}
