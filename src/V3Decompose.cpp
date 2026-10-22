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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Decompose.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

वग्ग DecomposeBase VL_NOT_FINAL {
रक्षित:
    // TYPES

    // A component (element or member) of an aggregate type
    संरचना Component final {
        Ast_ग्रन्थिः_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        पूर्णाङ्क declIdx;  // The declared index of the element, if an array
        पूर्णाङ्क lsb;  // LSB in the whole value, if packed
        पूर्णाङ्क msb;  // MSB in the whole value, if packed
    };

    संरचना Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    संरचना Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        पूर्णाङ्क lsb;  // The first bit covered of this end
        पूर्णाङ्क otherLsb;  // The first bit covered of the other end
        पूर्णाङ्क width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    संरचना Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_ग्रन्थिः_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        तार्किक splittable = सच्च;  // Can be split (there is no reason not to)
        तार्किक wantsSplit = मिच्छा;  // Should be split (if possible, do split)
        तार्किक split = मिच्छा;  // Place was split
        तार्किक queued = मिच्छा;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    संरचना Select final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;  // The select
        Place* placep;  // The component
        पूर्णाङ्क lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    संरचना Assignment final {
        Ast_ग्रन्थिः_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

सार्वजनिक:
    // The state of the pass, shared by the steps
    वग्ग State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ध्रुव VNUser4InUse m_user4InUse;

    सार्वजनिक:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ग्रन्थिः_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

रक्षित:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    स्थिर तार्किक isUnpacked(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        सचे (VN_IS(dtypep, UnpackArrayDType)) पटिनिवत्त सच्च;
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        पटिनिवत्त structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    स्थिर तार्किक isPacked(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        सचे (ध्रुव AstPackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, PackArrayDType)) {
            पटिनिवत्त arrayp->subDTypep()->width() > 1;
        }
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        पटिनिवत्त structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    स्थिर तार्किक isSplittableType(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        सचे (isPacked(dtypep)) पटिनिवत्त v3Global.opt.fDecomposePacked();
        सचे (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            सचे (ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                अत्थाय (ध्रुव AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    सचे (mp->valuep()) पटिनिवत्त मिच्छा;
                }
            }
            पटिनिवत्त v3Global.opt.fDecomposeUnpacked();
        }
        पटिनिवत्त मिच्छा;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    स्थिर Ast_ग्रन्थिः_DType* dtypeOf(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) पटिनिवत्त refp->varp()->dtypep();
        पटिनिवत्त nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ध्रुव std::vector<Component>& dtypeComponents(Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        सचे (compsr.empty()) {
            सचे (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव arrayp = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                ध्रुव तार्किक packed = isPacked(dtypep);
                ध्रुव VNumRange range = arrayp->declRange();
                ध्रुव तार्किक rev = packed && range.ascending();  // Only for naming components
                Ast_ग्रन्थिः_DType* ध्रुव subp = arrayp->subDTypep();
                अत्थाय (पूर्णाङ्क i = 0; i < range.elements(); ++i) {
                    ध्रुव पूर्णाङ्क lsb = packed ? i * subp->width() : 0;
                    ध्रुव पूर्णाङ्क msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ध्रुव पूर्णाङ्क declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.प्रक्षिप_back({subp, nullptr, declIdx, lsb, msb});
                }
            } अञ्ञथा {
                ध्रुव AstStructDType* ध्रुव structp = VN_AS(dtypep, StructDType);
                ध्रुव तार्किक packed = isPacked(dtypep);
                अत्थाय (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ध्रुव पूर्णाङ्क lsb = packed ? mp->lsb() : 0;
                    ध्रुव पूर्णाङ्क msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.प्रक्षिप_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        पटिनिवत्त compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_ग्रन्थिः_DType* dtypep, पूर्णाङ्क bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ध्रुव std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ध्रुव auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ध्रुव Component& comp, पूर्णाङ्क b) {  //
                                             पटिनिवत्त b > comp.msb;
                                         });
        पटिनिवत्त it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ध्रुव std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        सचे (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        सचे (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        पटिनिवत्त childpr.get();
    }

    // Is 'nodep' cheap to clone
    स्थिर तार्किक isCheap(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Constants are cheap
        सचे (VN_IS(nodep, Const)) पटिनिवत्त सच्च;
        // So are resets
        सचे (VN_IS(nodep, CReset)) पटिनिवत्त सच्च;
        // Variable references are cheap for the most part
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            सचे (refp->varp()->isForced()) पटिनिवत्त मिच्छा;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            सचे (refp->varp()->isSc()) पटिनिवत्त मिच्छा;
            // Otherwise the path rooted here is cheap
            पटिनिवत्त सच्च;
        }
        // Bit selects only with a constant LSB
        सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            पटिनिवत्त VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        सचे (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(nodep, ArraySel)) {
            पटिनिवत्त isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        सचे (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {  //
            पटिनिवत्त isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        सचे (ध्रुव AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {  //
            पटिनिवत्त isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        पटिनिवत्त मिच्छा;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

वग्ग DecomposeRecord final : सार्वजनिक VNVisitorConst, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ध्रुव VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ग्रन्थिः_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, ध्रुव Ast_ग्रन्थिः* wherep, ध्रुव वर्ण* reasonp) {
        // Only warn if user requested splitting
        सचे (!varp->attrSplitVar()) पटिनिवत्त;
        // A packed variable might still be split by V3BitBlast, which warns if not
        सचे (isPacked(varp->dtypep())) पटिनिवत्त;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, सच्च);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        सचे (!varp->user1()) {
            ध्रुव तार्किक eligible = [&]() {
                // Check data type
                सचे (!isSplittableType(varp->dtypep())) पटिनिवत्त मिच्छा;
                // Check properties
                ध्रुव वर्ण* ध्रुव reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                सचे (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                पटिनिवत्त !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        पटिनिवत्त varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ध्रुव AstVarRef* refp) {
        AstVarScope* ध्रुव vscp = refp->varScopep();
        सचे (!isEligible(vscp->varp())) पटिनिवत्त nullptr;
        Place& place = m_state.places(vscp);
        सचे (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.प्रक्षिप_back(&place);
        }
        पटिनिवत्त &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    स्थिर शून्य block(Place* placep, ध्रुव Ast_ग्रन्थिः* wherep) {
        placep->splittable = मिच्छा;
        // Warn on the variable only, a component is split as far as possible, as requested
        सचे (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        सचे (AstArraySel* ध्रुव selp = VN_CAST(exprp, ArraySel)) {
            सचे (Place* ध्रुव fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ध्रुव AstConst* ध्रुव bitp = VN_CAST(selp->bitp(), Const);
                सचे (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ध्रुव childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = सच्च;
                    chain.प्रक्षिप_back({selp, childp, 0});
                    पटिनिवत्त childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            पटिनिवत्त nullptr;
        }
        // Struct selects correspond one to one with a component select
        सचे (AstStructSel* ध्रुव selp = VN_CAST(exprp, StructSel)) {
            सचे (Place* ध्रुव fromp = resolveChain(selp->fromp(), chain)) {
                सचे (!isUnpacked(fromp->dtypep)) पटिनिवत्त nullptr;
                AstStructDType* ध्रुव structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                सचे (!structp->user1SetOnce()) {
                    ध्रुव std::vector<Component>& compsr = dtypeComponents(structp);
                    अत्थाय (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ध्रुव Ast_ग्रन्थिः* ध्रुव memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ध्रुव childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = सच्च;
                chain.प्रक्षिप_back({selp, childp, 0});
                पटिनिवत्त childp;
            }
            पटिनिवत्त nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        सचे (AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            सचे (!placep) {
                iterateConst(selp->lsbp());
                पटिनिवत्त nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ध्रुव AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const);
            सचे (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                पटिनिवत्त nullptr;
            }
            // Descend into the Place containing the bits, until exact
            पूर्णाङ्क lsb = lsbp->toSInt();
            पूर्णाङ्क msb = lsb + selp->widthConst() - 1;
            याव (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                सचे (!isPacked(placep->dtypep)) पटिनिवत्त nullptr;
                ध्रुव size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                सचे (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    पटिनिवत्त nullptr;
                }
                placep->wantsSplit = सच्च;
                ध्रुव Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.प्रक्षिप_back({selp, placep, lsb});
            }
            पटिनिवत्त placep;
        }
        // Base case
        सचे (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) पटिनिवत्त placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        पटिनिवत्त nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    शून्य referencedWhole(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        सचे (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            पटिनिवत्त;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    शून्य visitSelect(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ध्रुव placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        सचे (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        सचे (!chain.empty()) m_state.chains.प्रक्षिप_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    स्थिर तार्किक hasReversedRange(ध्रुव Ast_ग्रन्थिः_DType* aDTypep, ध्रुव Ast_ग्रन्थिः_DType* bDTypep) {
        ध्रुव AstUnpackArrayDType* ध्रुव aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ध्रुव AstUnpackArrayDType* ध्रुव bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        सचे (!aArrayp || !bArrayp) पटिनिवत्त मिच्छा;
        सचे (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) पटिनिवत्त सच्च;
        पटिनिवत्त hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    शून्य visitAssignment(Ast_ग्रन्थिः_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ध्रुव तार्किक reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        सचे (!m_assignp) पटिनिवत्त;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        Place* ध्रुव lPlacep = lhsp->user1u().to<Place*>();
        Place* ध्रुव rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ध्रुव तार्किक splittable = [&]() {
            // Assignment between two Places is splittable
            सचे (lPlacep && rPlacep) पटिनिवत्त सच्च;
            // If the LHS is a Place, the RHS must be a splittable expression
            सचे (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                सचे (isPacked(lPlacep->dtypep)) पटिनिवत्त सच्च;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                पटिनिवत्त isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            सचे (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                सचे (isPacked(rPlacep->dtypep)) पटिनिवत्त सच्च;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                पटिनिवत्त isCheap(lhsp);
            }
            // Otherwise not splittable
            पटिनिवत्त मिच्छा;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        सचे (!splittable) {
            सचे (rPlacep) block(rPlacep, rhsp);
            सचे (lPlacep) block(lPlacep, lhsp);
            पटिनिवत्त;
        }

        // If both sides are places, record the whole copies for the Decision phase
        सचे (lPlacep && rPlacep) {
            ध्रुव पूर्णाङ्क width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.प्रक्षिप_back({rPlacep, 0, 0, width});
            rPlacep->copies.प्रक्षिप_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.प्रक्षिप_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_DType*) override {}  // No references in data types
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    शून्य visit(AstArraySel* nodep) override { visitSelect(nodep); }
    शून्य visit(AstStructSel* nodep) override { visitSelect(nodep); }
    शून्य visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    शून्य visit(AstAssign* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    शून्य visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        सचे (Place* ध्रुव placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ध्रुव varp = nodep->varp();
        सचे (!isEligible(varp)) पटिनिवत्त;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        अत्थाय (Place* ध्रुव placep : m_state.rootps) {
            सचे (!isEligible(placep->vscp->varp())) placep->splittable = मिच्छा;
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

वग्ग DecomposeDecision final : सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ध्रुव VNUser1InUse m_user1InUse;

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
    स्थिर तार्किक canSplit(ध्रुव Place* placep) {
        // Not if not splittable
        सचे (!placep->splittable) पटिनिवत्त मिच्छा;
        // Not if already split
        सचे (placep->split) पटिनिवत्त मिच्छा;
        // Original variable or its parent was already split
        पटिनिवत्त !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    शून्य enqueue(Place* placep) {
        सचे (placep->queued || !placep->wantsSplit || !canSplit(placep)) पटिनिवत्त;
        placep->queued = सच्च;
        m_worklist.प्रक्षिप_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    शून्य createComponents(Place* placep) {
        AstVarScope* ध्रुव vscp = placep->vscp;
        AstVar* ध्रुव varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        सचे (varps.empty()) {
            ध्रुव तार्किक packed = isPacked(varp->dtypep());
            सचे (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } अञ्ञथा {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_पङ्क्तिः* ध्रुव flp = varp->fileline();
            ध्रुव VVarType varType = varp->varType();
            ध्रुव std::string name = varp->name();
            अत्थाय (ध्रुव Component& comp : dtypeComponents(varp->dtypep())) {
                ध्रुव std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(comp.declIdx) + "__KET__";
                AstVar* ध्रुव newp = नूतन AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.प्रक्षिप_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ध्रुव scopep = vscp->scopep();
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
        अत्थाय (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ध्रुव newp = नूतन AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    शून्य addCopy(Place* placep, ध्रुव Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        सचे (!placep->splittable) पटिनिवत्त;
        // Add the copy to the Place
        placep->copies.प्रक्षिप_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        सचे (!isPacked(placep->dtypep)) पटिनिवत्त;
        सचे (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = सच्च;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    शून्य newCopy(Place* ap, पूर्णाङ्क aLsb, Place* bp, पूर्णाङ्क bLsb, पूर्णाङ्क width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    शून्य splitCopy(Place* placep, ध्रुव Copy& copy) {
        Place* ध्रुव otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        सचे (isPacked(placep->dtypep)) {
            ध्रुव std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ध्रुव पूर्णाङ्क lsb = copy.lsb;
            ध्रुव पूर्णाङ्क msb = copy.lsb + copy.width - 1;
            ध्रुव size_t first = componentIndex(placep->dtypep, lsb);
            अत्थाय (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ध्रुव Component& comp = comps[i];
                ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            पटिनिवत्त;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = सच्च;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ध्रुव size_t size = dtypeComponents(placep->dtypep).size();
        अत्थाय (size_t i = 0; i < size; ++i) {
            Place* ध्रुव ap = componentOf(placep, i);
            सचे (!ap->splittable) अनुवर्त;
            Place* ध्रुव bp = componentOf(otherp, i);
            सचे (!bp->splittable) अनुवर्त;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    शून्य splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = सच्च;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ध्रुव size_t size = dtypeComponents(placep->dtypep).size();
        अत्थाय (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        अत्थाय (ध्रुव Copy& copy : placep->copies) {
            सचे (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        अत्थाय (Place* ध्रुव placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        याव (!m_worklist.empty()) {
            Place* ध्रुव placep = m_worklist.back();
            m_worklist.निष्कासय_back();
            placep->queued = मिच्छा;
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

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

वग्ग DecomposeRewrite final : सार्वजनिक VNDeleter, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ध्रुव VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    स्थिर AstVarRef* newRef(File_पङ्क्तिः* flp, ध्रुव Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        पटिनिवत्त नूतन AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    शून्य rewriteChain(ध्रुव std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ध्रुव Select* selectp = nullptr;
        अत्थाय (ध्रुव Select& select : chain) {
            सचे (select.placep->parentp->split) selectp = &select;
        }
        सचे (!selectp) पटिनिवत्त;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ध्रुव VAccess access = [&]() {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* rootp = chain.front().exprp;
            याव (सच्च) {
                सचे (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } अञ्ञथा सचे (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } अञ्ञथा सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } अञ्ञथा {
                    पटिनिवत्त VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = newRef(flp, selectp->placep, access);
        सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
            ध्रुव पूर्णाङ्क width = selp->widthConst();
            सचे (width != newp->width()) newp = नूतन AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ग्रन्थिः_व्यञ्जकम्* newSelect(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtypep, size_t idx) {
        File_पङ्क्तिः* ध्रुव flp = fromp->fileline();
        ध्रुव Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        सचे (isPacked(dtypep)) {
            AstSel* ध्रुव selp = नूतन AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            पटिनिवत्त selp;
        }
        // If unpacked array, it's an ArraySel
        सचे (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ध्रुव पूर्णाङ्क i = static_cast<पूर्णाङ्क>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            पटिनिवत्त नूतन AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ध्रुव selp = नूतन AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        पटिनिवत्त selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    स्थिर std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> concatTerms(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> termps;
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> राशिः{nodep};
        याव (!राशिः.empty()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = राशिः.back();
            राशिः.निष्कासय_back();
            सचे (AstConcat* ध्रुव concatp = VN_CAST(exprp, Concat)) {
                राशिः.प्रक्षिप_back(concatp->lhsp());
                राशिः.प्रक्षिप_back(concatp->rhsp());
                अनुवर्त;
            }
            सचे (AstReplicate* ध्रुव repp = VN_CAST(exprp, Replicate)) {
                ध्रुव AstConst* ध्रुव countp = VN_AS(repp->countp(), Const);
                अत्थाय (uint32_t i = 0; i < countp->toUInt(); ++i) राशिः.प्रक्षिप_back(repp->srcp());
                अनुवर्त;
            }
            termps.प्रक्षिप_back(exprp);
        }
        पटिनिवत्त termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    स्थिर तार्किक isSliceable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            पटिनिवत्त VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        सचे (ध्रुव AstConcat* ध्रुव catp = VN_CAST(nodep, Concat)) {
            पटिनिवत्त isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        सचे (ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            पटिनिवत्त isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        सचे (ध्रुव AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
            पटिनिवत्त isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        सचे (ध्रुव AstOr* ध्रुव orp = VN_CAST(nodep, Or)) {
            पटिनिवत्त isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        सचे (ध्रुव AstXor* ध्रुव xorp = VN_CAST(nodep, Xor)) {
            पटिनिवत्त isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        सचे (ध्रुव AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {
            पटिनिवत्त isSliceable(extp->lhsp());
        }
        सचे (ध्रुव AstNot* ध्रुव notp = VN_CAST(nodep, Not)) {  //
            पटिनिवत्त isSliceable(notp->lhsp());
        }
        सचे (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep, Replicate)) {
            पटिनिवत्त isSliceable(repp->srcp());
        }
        पटिनिवत्त isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newSlice(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क lsb, पूर्णाङ्क width) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        // A constant, as a new constant of the bits
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            V3_सङ्ख्या अङ्कः{nodep, width};
            अङ्कः.opSel(constp->अङ्कः(), lsb + width - 1, lsb);
            पटिनिवत्त नूतन AstConst{flp, अङ्कः};
        }
        // A constant bit select, from the bits of what it selects from
        सचे (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            सचे (ध्रुव AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const)) {
                पटिनिवत्त newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        सचे (AstConcat* ध्रुव catp = VN_CAST(nodep, Concat)) {
            ध्रुव पूर्णाङ्क rWidth = catp->rhsp()->width();
            ध्रुव पूर्णाङ्क msb = lsb + width - 1;
            सचे (msb < rWidth) पटिनिवत्त newSlice(catp->rhsp(), lsb, width);
            सचे (lsb >= rWidth) पटिनिवत्त newSlice(catp->lhsp(), lsb - rWidth, width);
            पटिनिवत्त नूतन AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        सचे (AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            पटिनिवत्त नूतन AstCond{flp, condp->condp()->clone_वृक्षः_Pure(मिच्छा),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        सचे (AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
            पटिनिवत्त नूतन AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        सचे (AstOr* ध्रुव orp = VN_CAST(nodep, Or)) {
            पटिनिवत्त नूतन AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        सचे (AstXor* ध्रुव xorp = VN_CAST(nodep, Xor)) {
            पटिनिवत्त नूतन AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        सचे (AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {
            ध्रुव पूर्णाङ्क srcWidth = extp->lhsp()->width();
            ध्रुव पूर्णाङ्क msb = lsb + width - 1;
            सचे (msb < srcWidth) पटिनिवत्त newSlice(extp->lhsp(), lsb, width);
            सचे (lsb >= srcWidth) पटिनिवत्त नूतन AstConst{flp, AstConst::Widthed_मानम्{}, width, 0};
            पटिनिवत्त नूतन AstConcat{
                flp, नूतन AstConst{flp, AstConst::Widthed_मानम्{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        सचे (AstNot* ध्रुव notp = VN_CAST(nodep, Not)) {
            पटिनिवत्त नूतन AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        सचे (AstReplicate* ध्रुव repp = VN_CAST(nodep, Replicate)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcp = repp->srcp();
            ध्रुव पूर्णाङ्क srcWidth = srcp->width();
            ध्रुव पूर्णाङ्क msb = lsb + width - 1;
            Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
            अत्थाय (पूर्णाङ्क copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ध्रुव पूर्णाङ्क partLsb = std::max(lsb, copyLsb) - copyLsb;
                ध्रुव पूर्णाङ्क partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
            }
            पटिनिवत्त resultp;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = nodep->clone_वृक्षः_Pure(मिच्छा);
        सचे (lsb == 0 && width == nodep->width()) पटिनिवत्त clonep;
        पटिनिवत्त नूतन AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> newAssignRhsps(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        सचे (AstCReset* ध्रुव cresetp = VN_CAST(nodep, CReset)) {
            अत्थाय (ध्रुव Component& comp : compsr) {
                AstCReset* ध्रुव newp = cresetp->clone_वृक्षः(मिच्छा);
                newp->dtypep(comp.dtypep);
                valueps.प्रक्षिप_back(newp);
            }
            पटिनिवत्त valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        सचे (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            पूर्णाङ्क lsb = 0;  // LSB of the current term
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : concatTerms(nodep)) {
                ध्रुव पूर्णाङ्क msb = lsb + termp->width() - 1;
                ध्रुव size_t first = componentIndex(dtypep, lsb);
                अत्थाय (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ध्रुव Component& comp = compsr[i];
                    ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                    ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                    File_पङ्क्तिः* ध्रुव flp = termp->fileline();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? नूतन AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            पटिनिवत्त valueps;
        }

        // Otherwise unpacked, so select each component
        अत्थाय (size_t i = 0; i < compsr.size(); ++i) {
            valueps.प्रक्षिप_back(newSelect(nodep->clone_वृक्षः(मिच्छा), dtypep, i));
        }
        पटिनिवत्त valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ग्रन्थिः_व्यञ्जकम्* newBits(File_पङ्क्तिः* flp, ध्रुव Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        सचे (!placep->split) {
            AstVarRef* ध्रुव refp = newRef(flp, placep, VAccess::READ);
            सचे (lsb == 0 && msb == dtypep->width() - 1) पटिनिवत्त refp;
            पटिनिवत्त नूतन AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ध्रुव std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        ध्रुव size_t first = componentIndex(dtypep, lsb);
        अत्थाय (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ध्रुव Component& comp = comps[i];
            ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ध्रुव childp = placep->childrenp.at(i).get();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        पटिनिवत्त resultp;
    }

    स्थिर शून्य addNewAssign(Ast_ग्रन्थिः_Assign* origp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    शून्य expandSS(Ast_ग्रन्थिः_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        ध्रुव std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        सचे (isPacked(rPlacep->dtypep)) {
            अत्थाय (size_t i = 0; i < compsr.size(); ++i) {
                Place* ध्रुव lChildp = lPlacep->childrenp.at(i).get();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } अञ्ञथा {
            अत्थाय (size_t i = 0; i < compsr.size(); ++i) {
                Place* ध्रुव lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    शून्य expandSN(Ast_ग्रन्थिः_Assign* origp, Place* lPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        अत्थाय (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    शून्य expandNS(Ast_ग्रन्थिः_Assign* origp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Place* rPlacep) {
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        सचे (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            पटिनिवत्त;
        }
        // Unpacked of the same shape, component-wise
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ध्रुव तार्किक readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ध्रुव VNUser2InUse user2InUse;
            पटिनिवत्त lhsp->exists([](ध्रुव AstVarRef* refp) {
                AstVarScope* ध्रुव vscp = refp->varScopep();
                सचे (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                सचे (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                पटिनिवत्त vscp->user2() == 3;
            });
        }();
        सचे (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ध्रुव scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, नूतन AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, नूतन AstVarRef{flp, vscp, VAccess::READ});
            पटिनिवत्त;
        }
        ध्रुव size_t size = dtypeComponents(dtypep).size();
        अत्थाय (size_t i = 0; i < size; ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newLhsp = newSelect(lhsp->clone_वृक्षः(मिच्छा), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    शून्य expand(Ast_ग्रन्थिः_Assign* origp,  //
                Place* lPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,  //
                Place* rPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        // A Place not split is referenced
        सचे (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        सचे (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        सचे (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } अञ्ञथा सचे (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } अञ्ञथा सचे (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } अञ्ञथा {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    तार्किक isSplitBetween(ध्रुव Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        याव (placep->split) {
            ध्रुव size_t idx = componentIndex(placep->dtypep, lsb);
            सचे (idx != componentIndex(placep->dtypep, msb)) पटिनिवत्त सच्च;
            ध्रुव Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        पटिनिवत्त मिच्छा;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    शून्य hoistTerms(Ast_ग्रन्थिः_Assign* origp, ध्रुव Place* lPlacep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp();
        ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ध्रुव VNUser2InUse user2InUse;
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ध्रुव scopep = lPlacep->rootp->vscp->scopep();
        पूर्णाङ्क lsb = 0;  // LSB of the current term
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) {
            ध्रुव पूर्णाङ्क tLsb = lsb;
            ध्रुव पूर्णाङ्क tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            सचे (!termp->user2()) अनुवर्त;
            // Cheap to select from
            सचे (isCheap(termp)) {
                termp->user2(0);
                अनुवर्त;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            सचे (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                अनुवर्त;
            }
            // Bits can be selected without evaluating it more than once
            सचे (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                अनुवर्त;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_पङ्क्तिः* ध्रुव flp = termp->fileline();
            AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ध्रुव tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    स्थिर शून्य markComponents(ध्रुव Place* placep) {
        अत्थाय (ध्रुव std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(सच्च);
            सचे (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    शून्य rewriteAssignment(ध्रुव Assignment& assignment) {
        Place* ध्रुव lPlacep = assignment.lPlacep;
        Place* ध्रुव rPlacep = assignment.rPlacep;
        ध्रुव तार्किक lSplit = lPlacep && lPlacep->split;
        ध्रुव तार्किक rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        सचे (!lSplit && !rSplit) पटिनिवत्त;

        Ast_ग्रन्थिः_Assign* ध्रुव origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        सचे (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ध्रुव VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp();
            सचे (rhsp->exists([](ध्रुव AstVarRef* refp) { पटिनिवत्त refp->varScopep()->user2(); })) {
                File_पङ्क्तिः* ध्रुव flp = rhsp->fileline();
                AstScope* ध्रुव scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ध्रुव tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        सचे (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = origp->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        सचे (lSplit) VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
        सचे (rSplit) VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        सचे (!scopep->user1p()) scopep->user1p(scopep->comboActivep(सच्च));
        पटिनिवत्त VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    शून्य driveFromComponents(Place* placep, AstActive* activep) {
        सचे (!placep->split) पटिनिवत्त;
        AstVarScope* ध्रुव vscp = placep->vscp;
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
        अत्थाय (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ध्रुव childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(नूतन AstAlways{नूतन AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        अत्थाय (ध्रुव std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        अत्थाय (ध्रुव Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        अत्थाय (Place* ध्रुव placep : m_state.rootps) {
            सचे (!placep->split) अनुवर्त;
            ध्रुव AstVarScope* ध्रुव vscp = placep->vscp;
            // Only traced ones at this point
            सचे (!(vscp->varp()->isTrace() && vscp->isTrace())) अनुवर्त;
            AstActive* ध्रुव activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

शून्य V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("decompose", 0, dump_वृक्षः_Either_Level() >= 3);
}
