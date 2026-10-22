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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Decompose.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

வகுப்பு DecomposeBase VL_NOT_FINAL {
பாதுகாக்கப்பட்ட:
    // TYPES

    // A component (element or member) of an aggregate type
    கட்டமைப்பு Component final {
        Ast_முனை_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        முழு declIdx;  // The declared index of the element, if an array
        முழு lsb;  // LSB in the whole value, if packed
        முழு msb;  // MSB in the whole value, if packed
    };

    கட்டமைப்பு Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    கட்டமைப்பு Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        முழு lsb;  // The first bit covered of this end
        முழு otherLsb;  // The first bit covered of the other end
        முழு width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    கட்டமைப்பு Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_முனை_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        தருக்க splittable = உண்மை;  // Can be split (there is no reason not to)
        தருக்க wantsSplit = பொய்;  // Should be split (if possible, do split)
        தருக்க split = பொய்;  // Place was split
        தருக்க queued = பொய்;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    கட்டமைப்பு Select final {
        Ast_முனை_கோவை* exprp;  // The select
        Place* placep;  // The component
        முழு lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    கட்டமைப்பு Assignment final {
        Ast_முனை_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

பொது:
    // The state of the pass, shared by the steps
    வகுப்பு State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        மாறிலி VNUser4InUse m_user4InUse;

    பொது:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_முனை_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

பாதுகாக்கப்பட்ட:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    நிலையான தருக்க isUnpacked(மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (VN_IS(dtypep, UnpackArrayDType)) திரும்பு உண்மை;
        மாறிலி AstStructDType* மாறிலி structp = VN_CAST(dtypep, StructDType);
        திரும்பு structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    நிலையான தருக்க isPacked(மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (மாறிலி AstPackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, PackArrayDType)) {
            திரும்பு arrayp->subDTypep()->width() > 1;
        }
        மாறிலி AstStructDType* மாறிலி structp = VN_CAST(dtypep, StructDType);
        திரும்பு structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    நிலையான தருக்க isSplittableType(மாறிலி Ast_முனை_DType* dtypep) {
        எனில் (isPacked(dtypep)) திரும்பு v3Global.opt.fDecomposePacked();
        எனில் (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            எனில் (மாறிலி AstStructDType* மாறிலி structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                க்கு (மாறிலி AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    எனில் (mp->valuep()) திரும்பு பொய்;
                }
            }
            திரும்பு v3Global.opt.fDecomposeUnpacked();
        }
        திரும்பு பொய்;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    நிலையான Ast_முனை_DType* dtypeOf(மாறிலி Ast_முனை_கோவை* nodep) {
        எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) திரும்பு refp->varp()->dtypep();
        திரும்பு nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    மாறிலி std::vector<Component>& dtypeComponents(Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        எனில் (compsr.empty()) {
            எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி arrayp = VN_CAST(dtypep, முனை_Array_DType)) {
                மாறிலி தருக்க packed = isPacked(dtypep);
                மாறிலி VNumRange range = arrayp->declRange();
                மாறிலி தருக்க rev = packed && range.ascending();  // Only for naming components
                Ast_முனை_DType* மாறிலி subp = arrayp->subDTypep();
                க்கு (முழு i = 0; i < range.elements(); ++i) {
                    மாறிலி முழு lsb = packed ? i * subp->width() : 0;
                    மாறிலி முழு msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    மாறிலி முழு declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.தள்ளு_back({subp, nullptr, declIdx, lsb, msb});
                }
            } இல்லையெனில் {
                மாறிலி AstStructDType* மாறிலி structp = VN_AS(dtypep, StructDType);
                மாறிலி தருக்க packed = isPacked(dtypep);
                க்கு (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    மாறிலி முழு lsb = packed ? mp->lsb() : 0;
                    மாறிலி முழு msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.தள்ளு_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        திரும்பு compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_முனை_DType* dtypep, முழு bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        மாறிலி std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        மாறிலி auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](மாறிலி Component& comp, முழு b) {  //
                                             திரும்பு b > comp.msb;
                                         });
        திரும்பு it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        மாறிலி std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        எனில் (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        எனில் (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        திரும்பு childpr.get();
    }

    // Is 'nodep' cheap to clone
    நிலையான தருக்க isCheap(மாறிலி Ast_முனை_கோவை* nodep) {
        // Constants are cheap
        எனில் (VN_IS(nodep, Const)) திரும்பு உண்மை;
        // So are resets
        எனில் (VN_IS(nodep, CReset)) திரும்பு உண்மை;
        // Variable references are cheap for the most part
        எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            எனில் (refp->varp()->isForced()) திரும்பு பொய்;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            எனில் (refp->varp()->isSc()) திரும்பு பொய்;
            // Otherwise the path rooted here is cheap
            திரும்பு உண்மை;
        }
        // Bit selects only with a constant LSB
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            திரும்பு VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        எனில் (மாறிலி AstArraySel* மாறிலி selp = VN_CAST(nodep, ArraySel)) {
            திரும்பு isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        எனில் (மாறிலி AstStructSel* மாறிலி selp = VN_CAST(nodep, StructSel)) {  //
            திரும்பு isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        எனில் (மாறிலி AstExtend* மாறிலி extp = VN_CAST(nodep, Extend)) {  //
            திரும்பு isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        திரும்பு பொய்;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

வகுப்பு DecomposeRecord final : பொது VNVisitorConst, பொது DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_முனை_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    நிலையான வெறுமை warnNoSplit(AstVar* varp, மாறிலி Ast_முனை* wherep, மாறிலி எழுத்து* reasonp) {
        // Only warn if user requested splitting
        எனில் (!varp->attrSplitVar()) திரும்பு;
        // A packed variable might still be split by V3BitBlast, which warns if not
        எனில் (isPacked(varp->dtypep())) திரும்பு;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, உண்மை);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    நிலையான தருக்க isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        எனில் (!varp->user1()) {
            மாறிலி தருக்க eligible = [&]() {
                // Check data type
                எனில் (!isSplittableType(varp->dtypep())) திரும்பு பொய்;
                // Check properties
                மாறிலி எழுத்து* மாறிலி reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                எனில் (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                திரும்பு !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        திரும்பு varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(மாறிலி AstVarRef* refp) {
        AstVarScope* மாறிலி vscp = refp->varScopep();
        எனில் (!isEligible(vscp->varp())) திரும்பு nullptr;
        Place& place = m_state.places(vscp);
        எனில் (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.தள்ளு_back(&place);
        }
        திரும்பு &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    நிலையான வெறுமை block(Place* placep, மாறிலி Ast_முனை* wherep) {
        placep->splittable = பொய்;
        // Warn on the variable only, a component is split as far as possible, as requested
        எனில் (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_முனை_கோவை* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        எனில் (AstArraySel* மாறிலி selp = VN_CAST(exprp, ArraySel)) {
            எனில் (Place* மாறிலி fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                மாறிலி AstConst* மாறிலி bitp = VN_CAST(selp->bitp(), Const);
                எனில் (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* மாறிலி childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = உண்மை;
                    chain.தள்ளு_back({selp, childp, 0});
                    திரும்பு childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            திரும்பு nullptr;
        }
        // Struct selects correspond one to one with a component select
        எனில் (AstStructSel* மாறிலி selp = VN_CAST(exprp, StructSel)) {
            எனில் (Place* மாறிலி fromp = resolveChain(selp->fromp(), chain)) {
                எனில் (!isUnpacked(fromp->dtypep)) திரும்பு nullptr;
                AstStructDType* மாறிலி structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                எனில் (!structp->user1SetOnce()) {
                    மாறிலி std::vector<Component>& compsr = dtypeComponents(structp);
                    க்கு (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                மாறிலி Ast_முனை* மாறிலி memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* மாறிலி childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = உண்மை;
                chain.தள்ளு_back({selp, childp, 0});
                திரும்பு childp;
            }
            திரும்பு nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        எனில் (AstSel* மாறிலி selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            எனில் (!placep) {
                iterateConst(selp->lsbp());
                திரும்பு nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            மாறிலி AstConst* மாறிலி lsbp = VN_CAST(selp->lsbp(), Const);
            எனில் (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                திரும்பு nullptr;
            }
            // Descend into the Place containing the bits, until exact
            முழு lsb = lsbp->toSInt();
            முழு msb = lsb + selp->widthConst() - 1;
            போது (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                எனில் (!isPacked(placep->dtypep)) திரும்பு nullptr;
                மாறிலி size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                எனில் (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    திரும்பு nullptr;
                }
                placep->wantsSplit = உண்மை;
                மாறிலி Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.தள்ளு_back({selp, placep, lsb});
            }
            திரும்பு placep;
        }
        // Base case
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef)) திரும்பு placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        திரும்பு nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    வெறுமை referencedWhole(Ast_முனை_கோவை* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        எனில் (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            திரும்பு;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    வெறுமை visitSelect(Ast_முனை_கோவை* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* மாறிலி placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        எனில் (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        எனில் (!chain.empty()) m_state.chains.தள்ளு_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    நிலையான தருக்க hasReversedRange(மாறிலி Ast_முனை_DType* aDTypep, மாறிலி Ast_முனை_DType* bDTypep) {
        மாறிலி AstUnpackArrayDType* மாறிலி aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        மாறிலி AstUnpackArrayDType* மாறிலி bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        எனில் (!aArrayp || !bArrayp) திரும்பு பொய்;
        எனில் (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) திரும்பு உண்மை;
        திரும்பு hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    வெறுமை visitAssignment(Ast_முனை_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        மாறிலி தருக்க reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        எனில் (!m_assignp) திரும்பு;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp();
        Place* மாறிலி lPlacep = lhsp->user1u().to<Place*>();
        Place* மாறிலி rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        மாறிலி தருக்க splittable = [&]() {
            // Assignment between two Places is splittable
            எனில் (lPlacep && rPlacep) திரும்பு உண்மை;
            // If the LHS is a Place, the RHS must be a splittable expression
            எனில் (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                எனில் (isPacked(lPlacep->dtypep)) திரும்பு உண்மை;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                திரும்பு isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            எனில் (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                எனில் (isPacked(rPlacep->dtypep)) திரும்பு உண்மை;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                திரும்பு isCheap(lhsp);
            }
            // Otherwise not splittable
            திரும்பு பொய்;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        எனில் (!splittable) {
            எனில் (rPlacep) block(rPlacep, rhsp);
            எனில் (lPlacep) block(lPlacep, lhsp);
            திரும்பு;
        }

        // If both sides are places, record the whole copies for the Decision phase
        எனில் (lPlacep && rPlacep) {
            மாறிலி முழு width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.தள்ளு_back({rPlacep, 0, 0, width});
            rPlacep->copies.தள்ளு_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.தள்ளு_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_DType*) override {}  // No references in data types
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    வெறுமை visit(AstArraySel* nodep) override { visitSelect(nodep); }
    வெறுமை visit(AstStructSel* nodep) override { visitSelect(nodep); }
    வெறுமை visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    வெறுமை visit(AstAssign* nodep) override { visitAssignment(nodep); }
    வெறுமை visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    வெறுமை visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    வெறுமை visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        எனில் (Place* மாறிலி placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (!isEligible(varp)) திரும்பு;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        க்கு (Place* மாறிலி placep : m_state.rootps) {
            எனில் (!isEligible(placep->vscp->varp())) placep->splittable = பொய்;
        }
    }

பொது:
    நிலையான வெறுமை apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

வகுப்பு DecomposeDecision final : பொது DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    std::vector<Place*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::vector<AstVar*>> m_splitVarps;
    VDouble0 m_statSplitUnpackedOrig;  // Original AstVars of unpacked type split
    VDouble0 m_statSplitPackedOrig;  // Original AstVars of packed type split
    VDouble0 m_statSplitUnpackedComp;  // Component AstVars of unpacked type split
    VDouble0 m_statSplitPackedComp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    நிலையான தருக்க canSplit(மாறிலி Place* placep) {
        // Not if not splittable
        எனில் (!placep->splittable) திரும்பு பொய்;
        // Not if already split
        எனில் (placep->split) திரும்பு பொய்;
        // Original variable or its parent was already split
        திரும்பு !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    வெறுமை enqueue(Place* placep) {
        எனில் (placep->queued || !placep->wantsSplit || !canSplit(placep)) திரும்பு;
        placep->queued = உண்மை;
        m_worklist.தள்ளு_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    வெறுமை createComponents(Place* placep) {
        AstVarScope* மாறிலி vscp = placep->vscp;
        AstVar* மாறிலி varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        எனில் (varps.empty()) {
            மாறிலி தருக்க packed = isPacked(varp->dtypep());
            எனில் (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } இல்லையெனில் {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_வரி* மாறிலி flp = varp->fileline();
            மாறிலி VVarType varType = varp->varType();
            மாறிலி std::string name = varp->name();
            க்கு (மாறிலி Component& comp : dtypeComponents(varp->dtypep())) {
                மாறிலி std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_முனை::encode_எண்(comp.declIdx) + "__KET__";
                AstVar* மாறிலி newp = புதிய AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.தள்ளு_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* மாறிலி scopep = vscp->scopep();
        File_வரி* மாறிலி flp = vscp->fileline();
        க்கு (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* மாறிலி newp = புதிய AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    வெறுமை addCopy(Place* placep, மாறிலி Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        எனில் (!placep->splittable) திரும்பு;
        // Add the copy to the Place
        placep->copies.தள்ளு_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        எனில் (!isPacked(placep->dtypep)) திரும்பு;
        எனில் (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = உண்மை;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    வெறுமை newCopy(Place* ap, முழு aLsb, Place* bp, முழு bLsb, முழு width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    வெறுமை splitCopy(Place* placep, மாறிலி Copy& copy) {
        Place* மாறிலி otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        எனில் (isPacked(placep->dtypep)) {
            மாறிலி std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            மாறிலி முழு lsb = copy.lsb;
            மாறிலி முழு msb = copy.lsb + copy.width - 1;
            மாறிலி size_t first = componentIndex(placep->dtypep, lsb);
            க்கு (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                மாறிலி Component& comp = comps[i];
                மாறிலி முழு partLsb = std::max(lsb, comp.lsb);
                மாறிலி முழு partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            திரும்பு;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = உண்மை;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        மாறிலி size_t size = dtypeComponents(placep->dtypep).size();
        க்கு (size_t i = 0; i < size; ++i) {
            Place* மாறிலி ap = componentOf(placep, i);
            எனில் (!ap->splittable) தொடர்;
            Place* மாறிலி bp = componentOf(otherp, i);
            எனில் (!bp->splittable) தொடர்;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    வெறுமை splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = உண்மை;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        மாறிலி size_t size = dtypeComponents(placep->dtypep).size();
        க்கு (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        க்கு (மாறிலி Copy& copy : placep->copies) {
            எனில் (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        க்கு (Place* மாறிலி placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        போது (!m_worklist.empty()) {
            Place* மாறிலி placep = m_worklist.back();
            m_worklist.எடு_back();
            placep->queued = பொய்;
            splitPlace(placep);
        }

        V3Stats::addStat("Optimizations, Decompose, unpacked variables split",
                         m_statSplitUnpackedOrig);
        V3Stats::addStat("Optimizations, Decompose, packed variables split",
                         m_statSplitPackedOrig);
        V3Stats::addStat("Optimizations, Decompose, unpacked components split further",
                         m_statSplitUnpackedComp);
        V3Stats::addStat("Optimizations, Decompose, packed components split further",
                         m_statSplitPackedComp);
    }

பொது:
    நிலையான வெறுமை apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

வகுப்பு DecomposeRewrite final : பொது VNDeleter, பொது DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    நிலையான AstVarRef* newRef(File_வரி* flp, மாறிலி Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        திரும்பு புதிய AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    வெறுமை rewriteChain(மாறிலி std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        மாறிலி Select* selectp = nullptr;
        க்கு (மாறிலி Select& select : chain) {
            எனில் (select.placep->parentp->split) selectp = &select;
        }
        எனில் (!selectp) திரும்பு;
        Ast_முனை_கோவை* மாறிலி exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        மாறிலி VAccess access = [&]() {
            மாறிலி Ast_முனை_கோவை* rootp = chain.front().exprp;
            போது (உண்மை) {
                எனில் (மாறிலி AstArraySel* மாறிலி selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } இல்லையெனில் எனில் (மாறிலி AstStructSel* மாறிலி selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } இல்லையெனில் எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } இல்லையெனில் {
                    திரும்பு VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_வரி* மாறிலி flp = exprp->fileline();
        Ast_முனை_கோவை* newp = newRef(flp, selectp->placep, access);
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(exprp, Sel)) {
            மாறிலி முழு width = selp->widthConst();
            எனில் (width != newp->width()) newp = புதிய AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_முனை_கோவை* newSelect(Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtypep, size_t idx) {
        File_வரி* மாறிலி flp = fromp->fileline();
        மாறிலி Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        எனில் (isPacked(dtypep)) {
            AstSel* மாறிலி selp = புதிய AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            திரும்பு selp;
        }
        // If unpacked array, it's an ArraySel
        எனில் (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            மாறிலி முழு i = static_cast<முழு>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            திரும்பு புதிய AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* மாறிலி selp = புதிய AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        திரும்பு selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    நிலையான std::vector<Ast_முனை_கோவை*> concatTerms(Ast_முனை_கோவை* nodep) {
        std::vector<Ast_முனை_கோவை*> termps;
        std::vector<Ast_முனை_கோவை*> அடுக்கு{nodep};
        போது (!அடுக்கு.empty()) {
            Ast_முனை_கோவை* மாறிலி exprp = அடுக்கு.back();
            அடுக்கு.எடு_back();
            எனில் (AstConcat* மாறிலி concatp = VN_CAST(exprp, Concat)) {
                அடுக்கு.தள்ளு_back(concatp->lhsp());
                அடுக்கு.தள்ளு_back(concatp->rhsp());
                தொடர்;
            }
            எனில் (AstReplicate* மாறிலி repp = VN_CAST(exprp, Replicate)) {
                மாறிலி AstConst* மாறிலி countp = VN_AS(repp->countp(), Const);
                க்கு (uint32_t i = 0; i < countp->toUInt(); ++i) அடுக்கு.தள்ளு_back(repp->srcp());
                தொடர்;
            }
            termps.தள்ளு_back(exprp);
        }
        திரும்பு termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    நிலையான தருக்க isSliceable(மாறிலி Ast_முனை_கோவை* nodep) {
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            திரும்பு VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        எனில் (மாறிலி AstConcat* மாறிலி catp = VN_CAST(nodep, Concat)) {
            திரும்பு isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        எனில் (மாறிலி AstCond* மாறிலி condp = VN_CAST(nodep, Cond)) {
            திரும்பு isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(nodep, And)) {
            திரும்பு isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        எனில் (மாறிலி AstOr* மாறிலி orp = VN_CAST(nodep, Or)) {
            திரும்பு isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        எனில் (மாறிலி AstXor* மாறிலி xorp = VN_CAST(nodep, Xor)) {
            திரும்பு isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        எனில் (மாறிலி AstExtend* மாறிலி extp = VN_CAST(nodep, Extend)) {
            திரும்பு isSliceable(extp->lhsp());
        }
        எனில் (மாறிலி AstNot* மாறிலி notp = VN_CAST(nodep, Not)) {  //
            திரும்பு isSliceable(notp->lhsp());
        }
        எனில் (மாறிலி AstReplicate* மாறிலி repp = VN_CAST(nodep, Replicate)) {
            திரும்பு isSliceable(repp->srcp());
        }
        திரும்பு isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    நிலையான Ast_முனை_கோவை* newSlice(Ast_முனை_கோவை* nodep, முழு lsb, முழு width) {
        File_வரி* மாறிலி flp = nodep->fileline();
        // A constant, as a new constant of the bits
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            V3_எண் இலக்கம்{nodep, width};
            இலக்கம்.opSel(constp->இலக்கம்(), lsb + width - 1, lsb);
            திரும்பு புதிய AstConst{flp, இலக்கம்};
        }
        // A constant bit select, from the bits of what it selects from
        எனில் (AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) {
            எனில் (மாறிலி AstConst* மாறிலி lsbp = VN_CAST(selp->lsbp(), Const)) {
                திரும்பு newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        எனில் (AstConcat* மாறிலி catp = VN_CAST(nodep, Concat)) {
            மாறிலி முழு rWidth = catp->rhsp()->width();
            மாறிலி முழு msb = lsb + width - 1;
            எனில் (msb < rWidth) திரும்பு newSlice(catp->rhsp(), lsb, width);
            எனில் (lsb >= rWidth) திரும்பு newSlice(catp->lhsp(), lsb - rWidth, width);
            திரும்பு புதிய AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        எனில் (AstCond* மாறிலி condp = VN_CAST(nodep, Cond)) {
            திரும்பு புதிய AstCond{flp, condp->condp()->clone_மரம்_Pure(பொய்),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        எனில் (AstAnd* மாறிலி andp = VN_CAST(nodep, And)) {
            திரும்பு புதிய AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        எனில் (AstOr* மாறிலி orp = VN_CAST(nodep, Or)) {
            திரும்பு புதிய AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        எனில் (AstXor* மாறிலி xorp = VN_CAST(nodep, Xor)) {
            திரும்பு புதிய AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        எனில் (AstExtend* மாறிலி extp = VN_CAST(nodep, Extend)) {
            மாறிலி முழு srcWidth = extp->lhsp()->width();
            மாறிலி முழு msb = lsb + width - 1;
            எனில் (msb < srcWidth) திரும்பு newSlice(extp->lhsp(), lsb, width);
            எனில் (lsb >= srcWidth) திரும்பு புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, width, 0};
            திரும்பு புதிய AstConcat{
                flp, புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        எனில் (AstNot* மாறிலி notp = VN_CAST(nodep, Not)) {
            திரும்பு புதிய AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        எனில் (AstReplicate* மாறிலி repp = VN_CAST(nodep, Replicate)) {
            Ast_முனை_கோவை* மாறிலி srcp = repp->srcp();
            மாறிலி முழு srcWidth = srcp->width();
            மாறிலி முழு msb = lsb + width - 1;
            Ast_முனை_கோவை* resultp = nullptr;
            க்கு (முழு copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                மாறிலி முழு partLsb = std::max(lsb, copyLsb) - copyLsb;
                மாறிலி முழு partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_முனை_கோவை* மாறிலி bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? புதிய AstConcat{flp, bitsp, resultp} : bitsp;
            }
            திரும்பு resultp;
        }
        Ast_முனை_கோவை* மாறிலி clonep = nodep->clone_மரம்_Pure(பொய்);
        எனில் (lsb == 0 && width == nodep->width()) திரும்பு clonep;
        திரும்பு புதிய AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_முனை_கோவை*> newAssignRhsps(Ast_முனை_கோவை* nodep, Ast_முனை_DType* dtypep) {
        மாறிலி std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_முனை_கோவை*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        எனில் (AstCReset* மாறிலி cresetp = VN_CAST(nodep, CReset)) {
            க்கு (மாறிலி Component& comp : compsr) {
                AstCReset* மாறிலி newp = cresetp->clone_மரம்(பொய்);
                newp->dtypep(comp.dtypep);
                valueps.தள்ளு_back(newp);
            }
            திரும்பு valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        எனில் (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            முழு lsb = 0;  // LSB of the current term
            க்கு (Ast_முனை_கோவை* மாறிலி termp : concatTerms(nodep)) {
                மாறிலி முழு msb = lsb + termp->width() - 1;
                மாறிலி size_t first = componentIndex(dtypep, lsb);
                க்கு (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    மாறிலி Component& comp = compsr[i];
                    மாறிலி முழு partLsb = std::max(lsb, comp.lsb);
                    மாறிலி முழு partMsb = std::min(msb, comp.msb);
                    File_வரி* மாறிலி flp = termp->fileline();
                    Ast_முனை_கோவை* மாறிலி sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? புதிய AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            திரும்பு valueps;
        }

        // Otherwise unpacked, so select each component
        க்கு (size_t i = 0; i < compsr.size(); ++i) {
            valueps.தள்ளு_back(newSelect(nodep->clone_மரம்(பொய்), dtypep, i));
        }
        திரும்பு valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_முனை_கோவை* newBits(File_வரி* flp, மாறிலி Place* placep, முழு lsb, முழு msb) {
        Ast_முனை_DType* மாறிலி dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        எனில் (!placep->split) {
            AstVarRef* மாறிலி refp = newRef(flp, placep, VAccess::READ);
            எனில் (lsb == 0 && msb == dtypep->width() - 1) திரும்பு refp;
            திரும்பு புதிய AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        மாறிலி std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_முனை_கோவை* resultp = nullptr;
        மாறிலி size_t first = componentIndex(dtypep, lsb);
        க்கு (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            மாறிலி Component& comp = comps[i];
            மாறிலி முழு partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            மாறிலி முழு partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* மாறிலி childp = placep->childrenp.at(i).get();
            Ast_முனை_கோவை* மாறிலி bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? புதிய AstConcat{flp, bitsp, resultp} : bitsp;
        }
        திரும்பு resultp;
    }

    நிலையான வெறுமை addNewAssign(Ast_முனை_Assign* origp, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    வெறுமை expandSS(Ast_முனை_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_வரி* மாறிலி flp = origp->fileline();
        மாறிலி std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        எனில் (isPacked(rPlacep->dtypep)) {
            க்கு (size_t i = 0; i < compsr.size(); ++i) {
                Place* மாறிலி lChildp = lPlacep->childrenp.at(i).get();
                Ast_முனை_கோவை* மாறிலி rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } இல்லையெனில் {
            க்கு (size_t i = 0; i < compsr.size(); ++i) {
                Place* மாறிலி lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    வெறுமை expandSN(Ast_முனை_Assign* origp, Place* lPlacep, Ast_முனை_கோவை* rhsp) {
        மாறிலி std::vector<Ast_முனை_கோவை*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        க்கு (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    வெறுமை expandNS(Ast_முனை_Assign* origp, Ast_முனை_கோவை* lhsp, Place* rPlacep) {
        File_வரி* மாறிலி flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        எனில் (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            திரும்பு;
        }
        // Unpacked of the same shape, component-wise
        Ast_முனை_DType* மாறிலி dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        மாறிலி தருக்க readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            மாறிலி VNUser2InUse user2InUse;
            திரும்பு lhsp->exists([](மாறிலி AstVarRef* refp) {
                AstVarScope* மாறிலி vscp = refp->varScopep();
                எனில் (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                எனில் (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                திரும்பு vscp->user2() == 3;
            });
        }();
        எனில் (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* மாறிலி scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* மாறிலி vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, புதிய AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, புதிய AstVarRef{flp, vscp, VAccess::READ});
            திரும்பு;
        }
        மாறிலி size_t size = dtypeComponents(dtypep).size();
        க்கு (size_t i = 0; i < size; ++i) {
            Ast_முனை_கோவை* மாறிலி newLhsp = newSelect(lhsp->clone_மரம்(பொய்), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    வெறுமை expand(Ast_முனை_Assign* origp,  //
                Place* lPlacep, Ast_முனை_கோவை* lhsp,  //
                Place* rPlacep, Ast_முனை_கோவை* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_வரி* மாறிலி flp = origp->fileline();
        // A Place not split is referenced
        எனில் (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        எனில் (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        எனில் (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } இல்லையெனில் எனில் (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } இல்லையெனில் எனில் (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } இல்லையெனில் {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    தருக்க isSplitBetween(மாறிலி Place* placep, முழு lsb, முழு msb) {
        போது (placep->split) {
            மாறிலி size_t idx = componentIndex(placep->dtypep, lsb);
            எனில் (idx != componentIndex(placep->dtypep, msb)) திரும்பு உண்மை;
            மாறிலி Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        திரும்பு பொய்;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    வெறுமை hoistTerms(Ast_முனை_Assign* origp, மாறிலி Place* lPlacep) {
        Ast_முனை_கோவை* மாறிலி rhsp = origp->rhsp();
        மாறிலி std::vector<Ast_முனை_கோவை*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        மாறிலி VNUser2InUse user2InUse;
        க்கு (Ast_முனை_கோவை* மாறிலி termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* மாறிலி scopep = lPlacep->rootp->vscp->scopep();
        முழு lsb = 0;  // LSB of the current term
        க்கு (Ast_முனை_கோவை* மாறிலி termp : termps) {
            மாறிலி முழு tLsb = lsb;
            மாறிலி முழு tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            எனில் (!termp->user2()) தொடர்;
            // Cheap to select from
            எனில் (isCheap(termp)) {
                termp->user2(0);
                தொடர்;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            எனில் (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                தொடர்;
            }
            // Bits can be selected without evaluating it more than once
            எனில் (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                தொடர்;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_வரி* மாறிலி flp = termp->fileline();
            AstVarScope* மாறிலி vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(புதிய AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* மாறிலி tmpRefp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(புதிய AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    நிலையான வெறுமை markComponents(மாறிலி Place* placep) {
        க்கு (மாறிலி std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(உண்மை);
            எனில் (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    வெறுமை rewriteAssignment(மாறிலி Assignment& assignment) {
        Place* மாறிலி lPlacep = assignment.lPlacep;
        Place* மாறிலி rPlacep = assignment.rPlacep;
        மாறிலி தருக்க lSplit = lPlacep && lPlacep->split;
        மாறிலி தருக்க rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        எனில் (!lSplit && !rSplit) திரும்பு;

        Ast_முனை_Assign* மாறிலி origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        எனில் (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            மாறிலி VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_முனை_கோவை* மாறிலி rhsp = origp->rhsp();
            எனில் (rhsp->exists([](மாறிலி AstVarRef* refp) { திரும்பு refp->varScopep()->user2(); })) {
                File_வரி* மாறிலி flp = rhsp->fileline();
                AstScope* மாறிலி scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* மாறிலி vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(புதிய AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* மாறிலி tmpRefp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(புதிய AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        எனில் (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_முனை_கோவை* மாறிலி lhsp = origp->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        எனில் (lSplit) VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
        எனில் (rSplit) VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(தள்ளு_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    நிலையான AstActive* comboActive(AstScope* scopep) {
        எனில் (!scopep->user1p()) scopep->user1p(scopep->comboActivep(உண்மை));
        திரும்பு VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    வெறுமை driveFromComponents(Place* placep, AstActive* activep) {
        எனில் (!placep->split) திரும்பு;
        AstVarScope* மாறிலி vscp = placep->vscp;
        File_வரி* மாறிலி flp = vscp->fileline();
        க்கு (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* மாறிலி childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_முனை_கோவை* மாறிலி lhsRefp = புதிய AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_முனை_கோவை* மாறிலி lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(புதிய AstAlways{புதிய AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        க்கு (மாறிலி std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        க்கு (மாறிலி Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        க்கு (Place* மாறிலி placep : m_state.rootps) {
            எனில் (!placep->split) தொடர்;
            மாறிலி AstVarScope* மாறிலி vscp = placep->vscp;
            // Only traced ones at this point
            எனில் (!(vscp->varp()->isTrace() && vscp->isTrace())) தொடர்;
            AstActive* மாறிலி activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

பொது:
    நிலையான வெறுமை apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

வெறுமை V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("decompose", 0, dump_மரம்_Either_Level() >= 3);
}
