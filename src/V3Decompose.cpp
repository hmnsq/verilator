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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Decompose.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

ଶ୍ରେଣୀ DecomposeBase VL_NOT_FINAL {
ସୁରକ୍ଷିତ:
    // TYPES

    // A component (element or member) of an aggregate type
    ସଂରଚନା Component final {
        Ast_ଗଣ୍ଠି_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା declIdx;  // The declared index of the element, if an array
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // LSB in the whole value, if packed
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb;  // MSB in the whole value, if packed
    };

    ସଂରଚନା Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ସଂରଚନା Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // The first bit covered of this end
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା otherLsb;  // The first bit covered of the other end
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ସଂରଚନା Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_ଗଣ୍ଠି_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        ତାର୍କିକ splittable = ସତ୍ୟ;  // Can be split (there is no reason not to)
        ତାର୍କିକ wantsSplit = ମିଥ୍ୟା;  // Should be split (if possible, do split)
        ତାର୍କିକ split = ମିଥ୍ୟା;  // Place was split
        ତାର୍କିକ queued = ମିଥ୍ୟା;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ସଂରଚନା Select final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp;  // The select
        Place* placep;  // The component
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ସଂରଚନା Assignment final {
        Ast_ଗଣ୍ଠି_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

ସର୍ବସାଧାରଣ:
    // The state of the pass, shared by the steps
    ଶ୍ରେଣୀ State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ଅଚଳ VNUser4InUse m_user4InUse;

    ସର୍ବସାଧାରଣ:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ଗଣ୍ଠି_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

ସୁରକ୍ଷିତ:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ସ୍ଥିର ତାର୍କିକ isUnpacked(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (VN_IS(dtypep, UnpackArrayDType)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଅଚଳ AstStructDType* ଅଚଳ structp = VN_CAST(dtypep, StructDType);
        ଫେରନ୍ତୁ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ସ୍ଥିର ତାର୍କିକ isPacked(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ଫେରନ୍ତୁ arrayp->subDTypep()->width() > 1;
        }
        ଅଚଳ AstStructDType* ଅଚଳ structp = VN_CAST(dtypep, StructDType);
        ଫେରନ୍ତୁ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ସ୍ଥିର ତାର୍କିକ isSplittableType(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଯଦି (isPacked(dtypep)) ଫେରନ୍ତୁ v3Global.opt.fDecomposePacked();
        ଯଦି (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ଯଦି (ଅଚଳ AstStructDType* ଅଚଳ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                ପାଇଁ (ଅଚଳ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ଯଦି (mp->valuep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
            }
            ଫେରନ୍ତୁ v3Global.opt.fDecomposeUnpacked();
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* dtypeOf(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) ଫେରନ୍ତୁ refp->varp()->dtypep();
        ଫେରନ୍ତୁ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ଅଚଳ std::vector<Component>& dtypeComponents(Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ଯଦି (compsr.empty()) {
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ arrayp = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                ଅଚଳ ତାର୍କିକ packed = isPacked(dtypep);
                ଅଚଳ VNumRange range = arrayp->declRange();
                ଅଚଳ ତାର୍କିକ rev = packed && range.ascending();  // Only for naming components
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = arrayp->subDTypep();
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < range.elements(); ++i) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = packed ? i * subp->width() : 0;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ଠେଲ_back({subp, nullptr, declIdx, lsb, msb});
                }
            } ଅନ୍ୟଥା {
                ଅଚଳ AstStructDType* ଅଚଳ structp = VN_AS(dtypep, StructDType);
                ଅଚଳ ତାର୍କିକ packed = isPacked(dtypep);
                ପାଇଁ (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = packed ? mp->lsb() : 0;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.ଠେଲ_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        ଫେରନ୍ତୁ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_ଗଣ୍ଠି_DType* dtypep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ଅଚଳ std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ଅଚଳ auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ଅଚଳ Component& comp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା b) {  //
                                             ଫେରନ୍ତୁ b > comp.msb;
                                         });
        ଫେରନ୍ତୁ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ଅଚଳ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        ଯଦି (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        ଯଦି (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        ଫେରନ୍ତୁ childpr.get();
    }

    // Is 'nodep' cheap to clone
    ସ୍ଥିର ତାର୍କିକ isCheap(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // Constants are cheap
        ଯଦି (VN_IS(nodep, Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // So are resets
        ଯଦି (VN_IS(nodep, CReset)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Variable references are cheap for the most part
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ଯଦି (refp->varp()->isForced()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ଯଦି (refp->varp()->isSc()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Otherwise the path rooted here is cheap
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Bit selects only with a constant LSB
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଫେରନ୍ତୁ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ selp = VN_CAST(nodep, ArraySel)) {
            ଫେରନ୍ତୁ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {  //
            ଫେରନ୍ତୁ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ଯଦି (ଅଚଳ AstExtend* ଅଚଳ extp = VN_CAST(nodep, Extend)) {  //
            ଫେରନ୍ତୁ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

ଶ୍ରେଣୀ DecomposeRecord final : ସର୍ବସାଧାରଣ VNVisitorConst, ସର୍ବସାଧାରଣ DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ଗଣ୍ଠି_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ସ୍ଥିର ଶୂନ୍ୟ warnNoSplit(AstVar* varp, ଅଚଳ Ast_ଗଣ୍ଠି* wherep, ଅଚଳ ଅକ୍ଷର* reasonp) {
        // Only warn if user requested splitting
        ଯଦି (!varp->attrSplitVar()) ଫେରନ୍ତୁ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ଯଦି (isPacked(varp->dtypep())) ଫେରନ୍ତୁ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, ସତ୍ୟ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ସ୍ଥିର ତାର୍କିକ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ଯଦି (!varp->user1()) {
            ଅଚଳ ତାର୍କିକ eligible = [&]() {
                // Check data type
                ଯଦି (!isSplittableType(varp->dtypep())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Check properties
                ଅଚଳ ଅକ୍ଷର* ଅଚଳ reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                ଯଦି (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ଫେରନ୍ତୁ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ଫେରନ୍ତୁ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ଅଚଳ AstVarRef* refp) {
        AstVarScope* ଅଚଳ vscp = refp->varScopep();
        ଯଦି (!isEligible(vscp->varp())) ଫେରନ୍ତୁ nullptr;
        Place& place = m_state.places(vscp);
        ଯଦି (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.ଠେଲ_back(&place);
        }
        ଫେରନ୍ତୁ &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ସ୍ଥିର ଶୂନ୍ୟ block(Place* placep, ଅଚଳ Ast_ଗଣ୍ଠି* wherep) {
        placep->splittable = ମିଥ୍ୟା;
        // Warn on the variable only, a component is split as far as possible, as requested
        ଯଦି (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        ଯଦି (AstArraySel* ଅଚଳ selp = VN_CAST(exprp, ArraySel)) {
            ଯଦି (Place* ଅଚଳ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ଅଚଳ AstConst* ଅଚଳ bitp = VN_CAST(selp->bitp(), Const);
                ଯଦି (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ଅଚଳ childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = ସତ୍ୟ;
                    chain.ଠେଲ_back({selp, childp, 0});
                    ଫେରନ୍ତୁ childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ଫେରନ୍ତୁ nullptr;
        }
        // Struct selects correspond one to one with a component select
        ଯଦି (AstStructSel* ଅଚଳ selp = VN_CAST(exprp, StructSel)) {
            ଯଦି (Place* ଅଚଳ fromp = resolveChain(selp->fromp(), chain)) {
                ଯଦି (!isUnpacked(fromp->dtypep)) ଫେରନ୍ତୁ nullptr;
                AstStructDType* ଅଚଳ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ଯଦି (!structp->user1SetOnce()) {
                    ଅଚଳ std::vector<Component>& compsr = dtypeComponents(structp);
                    ପାଇଁ (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ଅଚଳ childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = ସତ୍ୟ;
                chain.ଠେଲ_back({selp, childp, 0});
                ଫେରନ୍ତୁ childp;
            }
            ଫେରନ୍ତୁ nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            ଯଦି (!placep) {
                iterateConst(selp->lsbp());
                ଫେରନ୍ତୁ nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ଅଚଳ AstConst* ଅଚଳ lsbp = VN_CAST(selp->lsbp(), Const);
            ଯଦି (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ଫେରନ୍ତୁ nullptr;
            }
            // Descend into the Place containing the bits, until exact
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = lsbp->toSInt();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + selp->widthConst() - 1;
            ଯେତେବେଳେ (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                ଯଦି (!isPacked(placep->dtypep)) ଫେରନ୍ତୁ nullptr;
                ଅଚଳ size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ଯଦି (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    ଫେରନ୍ତୁ nullptr;
                }
                placep->wantsSplit = ସତ୍ୟ;
                ଅଚଳ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.ଠେଲ_back({selp, placep, lsb});
            }
            ଫେରନ୍ତୁ placep;
        }
        // Base case
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) ଫେରନ୍ତୁ placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        ଫେରନ୍ତୁ nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    ଶୂନ୍ୟ referencedWhole(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ଯଦି (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ଫେରନ୍ତୁ;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    ଶୂନ୍ୟ visitSelect(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ଅଚଳ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ଯଦି (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        ଯଦି (!chain.empty()) m_state.chains.ଠେଲ_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ସ୍ଥିର ତାର୍କିକ hasReversedRange(ଅଚଳ Ast_ଗଣ୍ଠି_DType* aDTypep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* bDTypep) {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ଯଦି (!aArrayp || !bArrayp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    ଶୂନ୍ୟ visitAssignment(Ast_ଗଣ୍ଠି_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ଅଚଳ ତାର୍କିକ reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        ଯଦି (!m_assignp) ଫେରନ୍ତୁ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();
        Place* ଅଚଳ lPlacep = lhsp->user1u().to<Place*>();
        Place* ଅଚଳ rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ଅଚଳ ତାର୍କିକ splittable = [&]() {
            // Assignment between two Places is splittable
            ଯଦି (lPlacep && rPlacep) ଫେରନ୍ତୁ ସତ୍ୟ;
            // If the LHS is a Place, the RHS must be a splittable expression
            ଯଦି (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ଯଦି (isPacked(lPlacep->dtypep)) ଫେରନ୍ତୁ ସତ୍ୟ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ଫେରନ୍ତୁ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ଯଦି (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ଯଦି (isPacked(rPlacep->dtypep)) ଫେରନ୍ତୁ ସତ୍ୟ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ଫେରନ୍ତୁ isCheap(lhsp);
            }
            // Otherwise not splittable
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ଯଦି (!splittable) {
            ଯଦି (rPlacep) block(rPlacep, rhsp);
            ଯଦି (lPlacep) block(lPlacep, lhsp);
            ଫେରନ୍ତୁ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ଯଦି (lPlacep && rPlacep) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.ଠେଲ_back({rPlacep, 0, 0, width});
            rPlacep->copies.ଠେଲ_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.ଠେଲ_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType*) override {}  // No references in data types
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override { visitSelect(nodep); }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override { visitSelect(nodep); }
    ଶୂନ୍ୟ visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    ଶୂନ୍ୟ visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ଯଦି (Place* ଅଚଳ placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!isEligible(varp)) ଫେରନ୍ତୁ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        ପାଇଁ (Place* ଅଚଳ placep : m_state.rootps) {
            ଯଦି (!isEligible(placep->vscp->varp())) placep->splittable = ମିଥ୍ୟା;
        }
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

ଶ୍ରେଣୀ DecomposeDecision final : ସର୍ବସାଧାରଣ DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ଅଚଳ VNUser1InUse m_user1InUse;

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
    ସ୍ଥିର ତାର୍କିକ canSplit(ଅଚଳ Place* placep) {
        // Not if not splittable
        ଯଦି (!placep->splittable) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Not if already split
        ଯଦି (placep->split) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Original variable or its parent was already split
        ଫେରନ୍ତୁ !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    ଶୂନ୍ୟ enqueue(Place* placep) {
        ଯଦି (placep->queued || !placep->wantsSplit || !canSplit(placep)) ଫେରନ୍ତୁ;
        placep->queued = ସତ୍ୟ;
        m_worklist.ଠେଲ_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    ଶୂନ୍ୟ createComponents(Place* placep) {
        AstVarScope* ଅଚଳ vscp = placep->vscp;
        AstVar* ଅଚଳ varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        ଯଦି (varps.empty()) {
            ଅଚଳ ତାର୍କିକ packed = isPacked(varp->dtypep());
            ଯଦି (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } ଅନ୍ୟଥା {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_ଧାଡ଼ି* ଅଚଳ flp = varp->fileline();
            ଅଚଳ VVarType varType = varp->varType();
            ଅଚଳ std::string name = varp->name();
            ପାଇଁ (ଅଚଳ Component& comp : dtypeComponents(varp->dtypep())) {
                ଅଚଳ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(comp.declIdx) + "__KET__";
                AstVar* ଅଚଳ newp = ନୂତନ AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.ଠେଲ_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ଅଚଳ scopep = vscp->scopep();
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        ପାଇଁ (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ଅଚଳ newp = ନୂତନ AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    ଶୂନ୍ୟ addCopy(Place* placep, ଅଚଳ Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ଯଦି (!placep->splittable) ଫେରନ୍ତୁ;
        // Add the copy to the Place
        placep->copies.ଠେଲ_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ଯଦି (!isPacked(placep->dtypep)) ଫେରନ୍ତୁ;
        ଯଦି (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = ସତ୍ୟ;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    ଶୂନ୍ୟ newCopy(Place* ap, ପୂର୍ଣ୍ଣସଂଖ୍ୟା aLsb, Place* bp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    ଶୂନ୍ୟ splitCopy(Place* placep, ଅଚଳ Copy& copy) {
        Place* ଅଚଳ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ଯଦି (isPacked(placep->dtypep)) {
            ଅଚଳ std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = copy.lsb;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = copy.lsb + copy.width - 1;
            ଅଚଳ size_t first = componentIndex(placep->dtypep, lsb);
            ପାଇଁ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ଅଚଳ Component& comp = comps[i];
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLsb = std::max(lsb, comp.lsb);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ଫେରନ୍ତୁ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = ସତ୍ୟ;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ଅଚଳ size_t size = dtypeComponents(placep->dtypep).size();
        ପାଇଁ (size_t i = 0; i < size; ++i) {
            Place* ଅଚଳ ap = componentOf(placep, i);
            ଯଦି (!ap->splittable) ଜାରି;
            Place* ଅଚଳ bp = componentOf(otherp, i);
            ଯଦି (!bp->splittable) ଜାରି;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    ଶୂନ୍ୟ splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = ସତ୍ୟ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ଅଚଳ size_t size = dtypeComponents(placep->dtypep).size();
        ପାଇଁ (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        ପାଇଁ (ଅଚଳ Copy& copy : placep->copies) {
            ଯଦି (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        ପାଇଁ (Place* ଅଚଳ placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ଯେତେବେଳେ (!m_worklist.empty()) {
            Place* ଅଚଳ placep = m_worklist.back();
            m_worklist.କାଢ଼_back();
            placep->queued = ମିଥ୍ୟା;
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

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

ଶ୍ରେଣୀ DecomposeRewrite final : ସର୍ବସାଧାରଣ VNDeleter, ସର୍ବସାଧାରଣ DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ସ୍ଥିର AstVarRef* newRef(File_ଧାଡ଼ି* flp, ଅଚଳ Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    ଶୂନ୍ୟ rewriteChain(ଅଚଳ std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ଅଚଳ Select* selectp = nullptr;
        ପାଇଁ (ଅଚଳ Select& select : chain) {
            ଯଦି (select.placep->parentp->split) selectp = &select;
        }
        ଯଦି (!selectp) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ଅଚଳ VAccess access = [&]() {
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rootp = chain.front().exprp;
            ଯେତେବେଳେ (ସତ୍ୟ) {
                ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ଅନ୍ୟଥା {
                    ଫେରନ୍ତୁ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = newRef(flp, selectp->placep, access);
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(exprp, Sel)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = selp->widthConst();
            ଯଦି (width != newp->width()) newp = ନୂତନ AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSelect(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtypep, size_t idx) {
        File_ଧାଡ଼ି* ଅଚଳ flp = fromp->fileline();
        ଅଚଳ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ଯଦି (isPacked(dtypep)) {
            AstSel* ଅଚଳ selp = ନୂତନ AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            ଫେରନ୍ତୁ selp;
        }
        // If unpacked array, it's an ArraySel
        ଯଦି (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            ଫେରନ୍ତୁ ନୂତନ AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ଅଚଳ selp = ନୂତନ AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ଫେରନ୍ତୁ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ସ୍ଥିର std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> concatTerms(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> termps;
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ଗଦା{nodep};
        ଯେତେବେଳେ (!ଗଦା.empty()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = ଗଦା.back();
            ଗଦା.କାଢ଼_back();
            ଯଦି (AstConcat* ଅଚଳ concatp = VN_CAST(exprp, Concat)) {
                ଗଦା.ଠେଲ_back(concatp->lhsp());
                ଗଦା.ଠେଲ_back(concatp->rhsp());
                ଜାରି;
            }
            ଯଦି (AstReplicate* ଅଚଳ repp = VN_CAST(exprp, Replicate)) {
                ଅଚଳ AstConst* ଅଚଳ countp = VN_AS(repp->countp(), Const);
                ପାଇଁ (uint32_t i = 0; i < countp->toUInt(); ++i) ଗଦା.ଠେଲ_back(repp->srcp());
                ଜାରି;
            }
            termps.ଠେଲ_back(exprp);
        }
        ଫେରନ୍ତୁ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ସ୍ଥିର ତାର୍କିକ isSliceable(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଫେରନ୍ତୁ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ଯଦି (ଅଚଳ AstConcat* ଅଚଳ catp = VN_CAST(nodep, Concat)) {
            ଫେରନ୍ତୁ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ଯଦି (ଅଚଳ AstCond* ଅଚଳ condp = VN_CAST(nodep, Cond)) {
            ଫେରନ୍ତୁ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(nodep, And)) {
            ଫେରନ୍ତୁ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ଯଦି (ଅଚଳ AstOr* ଅଚଳ orp = VN_CAST(nodep, Or)) {
            ଫେରନ୍ତୁ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ଯଦି (ଅଚଳ AstXor* ଅଚଳ xorp = VN_CAST(nodep, Xor)) {
            ଫେରନ୍ତୁ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ଯଦି (ଅଚଳ AstExtend* ଅଚଳ extp = VN_CAST(nodep, Extend)) {
            ଫେରନ୍ତୁ isSliceable(extp->lhsp());
        }
        ଯଦି (ଅଚଳ AstNot* ଅଚଳ notp = VN_CAST(nodep, Not)) {  //
            ଫେରନ୍ତୁ isSliceable(notp->lhsp());
        }
        ଯଦି (ଅଚଳ AstReplicate* ଅଚଳ repp = VN_CAST(nodep, Replicate)) {
            ଫେରନ୍ତୁ isSliceable(repp->srcp());
        }
        ଫେରନ୍ତୁ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSlice(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            V3_ସଂଖ୍ୟା ଅଙ୍କ{nodep, width};
            ଅଙ୍କ.opSel(constp->ଅଙ୍କ(), lsb + width - 1, lsb);
            ଫେରନ୍ତୁ ନୂତନ AstConst{flp, ଅଙ୍କ};
        }
        // A constant bit select, from the bits of what it selects from
        ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ lsbp = VN_CAST(selp->lsbp(), Const)) {
                ଫେରନ୍ତୁ newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ଯଦି (AstConcat* ଅଚଳ catp = VN_CAST(nodep, Concat)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rWidth = catp->rhsp()->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
            ଯଦି (msb < rWidth) ଫେରନ୍ତୁ newSlice(catp->rhsp(), lsb, width);
            ଯଦି (lsb >= rWidth) ଫେରନ୍ତୁ newSlice(catp->lhsp(), lsb - rWidth, width);
            ଫେରନ୍ତୁ ନୂତନ AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        ଯଦି (AstCond* ଅଚଳ condp = VN_CAST(nodep, Cond)) {
            ଫେରନ୍ତୁ ନୂତନ AstCond{flp, condp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        ଯଦି (AstAnd* ଅଚଳ andp = VN_CAST(nodep, And)) {
            ଫେରନ୍ତୁ ନୂତନ AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        ଯଦି (AstOr* ଅଚଳ orp = VN_CAST(nodep, Or)) {
            ଫେରନ୍ତୁ ନୂତନ AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        ଯଦି (AstXor* ଅଚଳ xorp = VN_CAST(nodep, Xor)) {
            ଫେରନ୍ତୁ ନୂତନ AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ଯଦି (AstExtend* ଅଚଳ extp = VN_CAST(nodep, Extend)) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcWidth = extp->lhsp()->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
            ଯଦି (msb < srcWidth) ଫେରନ୍ତୁ newSlice(extp->lhsp(), lsb, width);
            ଯଦି (lsb >= srcWidth) ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, width, 0};
            ଫେରନ୍ତୁ ନୂତନ AstConcat{
                flp, ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        ଯଦି (AstNot* ଅଚଳ notp = VN_CAST(nodep, Not)) {
            ଫେରନ୍ତୁ ନୂତନ AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ଯଦି (AstReplicate* ଅଚଳ repp = VN_CAST(nodep, Replicate)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ srcp = repp->srcp();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcWidth = srcp->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLsb = std::max(lsb, copyLsb) - copyLsb;
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? ନୂତନ AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ଫେରନ୍ତୁ resultp;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonep = nodep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (lsb == 0 && width == nodep->width()) ଫେରନ୍ତୁ clonep;
        ଫେରନ୍ତୁ ନୂତନ AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> newAssignRhsps(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଅଚଳ std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        ଯଦି (AstCReset* ଅଚଳ cresetp = VN_CAST(nodep, CReset)) {
            ପାଇଁ (ଅଚଳ Component& comp : compsr) {
                AstCReset* ଅଚଳ newp = cresetp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                newp->dtypep(comp.dtypep);
                valueps.ଠେଲ_back(newp);
            }
            ଫେରନ୍ତୁ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ଯଦି (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = 0;  // LSB of the current term
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp : concatTerms(nodep)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + termp->width() - 1;
                ଅଚଳ size_t first = componentIndex(dtypep, lsb);
                ପାଇଁ (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ଅଚଳ Component& comp = compsr[i];
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLsb = std::max(lsb, comp.lsb);
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partMsb = std::min(msb, comp.msb);
                    File_ଧାଡ଼ି* ଅଚଳ flp = termp->fileline();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? ନୂତନ AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ଫେରନ୍ତୁ valueps;
        }

        // Otherwise unpacked, so select each component
        ପାଇଁ (size_t i = 0; i < compsr.size(); ++i) {
            valueps.ଠେଲ_back(newSelect(nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), dtypep, i));
        }
        ଫେରନ୍ତୁ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newBits(File_ଧାଡ଼ି* flp, ଅଚଳ Place* placep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ଯଦି (!placep->split) {
            AstVarRef* ଅଚଳ refp = newRef(flp, placep, VAccess::READ);
            ଯଦି (lsb == 0 && msb == dtypep->width() - 1) ଫେରନ୍ତୁ refp;
            ଫେରନ୍ତୁ ନୂତନ AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ଅଚଳ std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        ଅଚଳ size_t first = componentIndex(dtypep, lsb);
        ପାଇଁ (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ଅଚଳ Component& comp = comps[i];
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ଅଚଳ childp = placep->childrenp.at(i).get();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? ନୂତନ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ଫେରନ୍ତୁ resultp;
    }

    ସ୍ଥିର ଶୂନ୍ୟ addNewAssign(Ast_ଗଣ୍ଠି_Assign* origp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    ଶୂନ୍ୟ expandSS(Ast_ଗଣ୍ଠି_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = origp->fileline();
        ଅଚଳ std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ଯଦି (isPacked(rPlacep->dtypep)) {
            ପାଇଁ (size_t i = 0; i < compsr.size(); ++i) {
                Place* ଅଚଳ lChildp = lPlacep->childrenp.at(i).get();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } ଅନ୍ୟଥା {
            ପାଇଁ (size_t i = 0; i < compsr.size(); ++i) {
                Place* ଅଚଳ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    ଶୂନ୍ୟ expandSN(Ast_ଗଣ୍ଠି_Assign* origp, Place* lPlacep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        ପାଇଁ (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    ଶୂନ୍ୟ expandNS(Ast_ଗଣ୍ଠି_Assign* origp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Place* rPlacep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ଯଦି (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            ଫେରନ୍ତୁ;
        }
        // Unpacked of the same shape, component-wise
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ଅଚଳ ତାର୍କିକ readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ଅଚଳ VNUser2InUse user2InUse;
            ଫେରନ୍ତୁ lhsp->exists([](ଅଚଳ AstVarRef* refp) {
                AstVarScope* ଅଚଳ vscp = refp->varScopep();
                ଯଦି (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ଯଦି (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ଫେରନ୍ତୁ vscp->user2() == 3;
            });
        }();
        ଯଦି (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ଅଚଳ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ଅଚଳ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ size_t size = dtypeComponents(dtypep).size();
        ପାଇଁ (size_t i = 0; i < size; ++i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newLhsp = newSelect(lhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    ଶୂନ୍ୟ expand(Ast_ଗଣ୍ଠି_Assign* origp,  //
                Place* lPlacep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp,  //
                Place* rPlacep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_ଧାଡ଼ି* ଅଚଳ flp = origp->fileline();
        // A Place not split is referenced
        ଯଦି (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        ଯଦି (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        ଯଦି (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ଅନ୍ୟଥା ଯଦି (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ଅନ୍ୟଥା ଯଦି (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ଅନ୍ୟଥା {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    ତାର୍କିକ isSplitBetween(ଅଚଳ Place* placep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb) {
        ଯେତେବେଳେ (placep->split) {
            ଅଚଳ size_t idx = componentIndex(placep->dtypep, lsb);
            ଯଦି (idx != componentIndex(placep->dtypep, msb)) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଅଚଳ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    ଶୂନ୍ୟ hoistTerms(Ast_ଗଣ୍ଠି_Assign* origp, ଅଚଳ Place* lPlacep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = origp->rhsp();
        ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ଅଚଳ VNUser2InUse user2InUse;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ଅଚଳ scopep = lPlacep->rootp->vscp->scopep();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = 0;  // LSB of the current term
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp : termps) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା tLsb = lsb;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ଯଦି (!termp->user2()) ଜାରି;
            // Cheap to select from
            ଯଦି (isCheap(termp)) {
                termp->user2(0);
                ଜାରି;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ଯଦି (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                ଜାରି;
            }
            // Bits can be selected without evaluating it more than once
            ଯଦି (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ଜାରି;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_ଧାଡ଼ି* ଅଚଳ flp = termp->fileline();
            AstVarScope* ଅଚଳ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ଅଚଳ tmpRefp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(ନୂତନ AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ସ୍ଥିର ଶୂନ୍ୟ markComponents(ଅଚଳ Place* placep) {
        ପାଇଁ (ଅଚଳ std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(ସତ୍ୟ);
            ଯଦି (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    ଶୂନ୍ୟ rewriteAssignment(ଅଚଳ Assignment& assignment) {
        Place* ଅଚଳ lPlacep = assignment.lPlacep;
        Place* ଅଚଳ rPlacep = assignment.rPlacep;
        ଅଚଳ ତାର୍କିକ lSplit = lPlacep && lPlacep->split;
        ଅଚଳ ତାର୍କିକ rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        ଯଦି (!lSplit && !rSplit) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ଯଦି (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ଅଚଳ VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = origp->rhsp();
            ଯଦି (rhsp->exists([](ଅଚଳ AstVarRef* refp) { ଫେରନ୍ତୁ refp->varScopep()->user2(); })) {
                File_ଧାଡ଼ି* ଅଚଳ flp = rhsp->fileline();
                AstScope* ଅଚଳ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ଅଚଳ vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ଅଚଳ tmpRefp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(ନୂତନ AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ଯଦି (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = origp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        ଯଦି (lSplit) VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
        ଯଦି (rSplit) VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ଠେଲ_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ସ୍ଥିର AstActive* comboActive(AstScope* scopep) {
        ଯଦି (!scopep->user1p()) scopep->user1p(scopep->comboActivep(ସତ୍ୟ));
        ଫେରନ୍ତୁ VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    ଶୂନ୍ୟ driveFromComponents(Place* placep, AstActive* activep) {
        ଯଦି (!placep->split) ଫେରନ୍ତୁ;
        AstVarScope* ଅଚଳ vscp = placep->vscp;
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        ପାଇଁ (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ଅଚଳ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsRefp = ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(ନୂତନ AstAlways{ନୂତନ AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        ପାଇଁ (ଅଚଳ std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        ପାଇଁ (ଅଚଳ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        ପାଇଁ (Place* ଅଚଳ placep : m_state.rootps) {
            ଯଦି (!placep->split) ଜାରି;
            ଅଚଳ AstVarScope* ଅଚଳ vscp = placep->vscp;
            // Only traced ones at this point
            ଯଦି (!(vscp->varp()->isTrace() && vscp->isTrace())) ଜାରି;
            AstActive* ଅଚଳ activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

ଶୂନ୍ୟ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("decompose", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
