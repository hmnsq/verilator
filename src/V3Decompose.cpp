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

वर्ग DecomposeBase VL_NOT_FINAL {
रक्षित:
    // TYPES

    // A component (element or member) of an aggregate type
    संरचना Component final {
        Ast_गाँठो_DType* dtypep;  // Type of the component
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
        Ast_गाँठो_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        तार्किक splittable = सत्य;  // Can be split (there is no reason not to)
        तार्किक wantsSplit = असत्य;  // Should be split (if possible, do split)
        तार्किक split = असत्य;  // Place was split
        तार्किक queued = असत्य;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    संरचना Select final {
        Ast_गाँठो_अभिव्यक्ति* exprp;  // The select
        Place* placep;  // The component
        पूर्णाङ्क lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    संरचना Assignment final {
        Ast_गाँठो_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

सार्वजनिक:
    // The state of the pass, shared by the steps
    वर्ग State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        अचर VNUser4InUse m_user4InUse;

    सार्वजनिक:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_गाँठो_DType, std::vector<Component>> dtypeComponents;
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
    स्थिर तार्किक isUnpacked(अचर Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (VN_IS(dtypep, UnpackArrayDType)) फर्काऊ सत्य;
        अचर AstStructDType* अचर structp = VN_CAST(dtypep, StructDType);
        फर्काऊ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    स्थिर तार्किक isPacked(अचर Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (अचर AstPackArrayDType* अचर arrayp = VN_CAST(dtypep, PackArrayDType)) {
            फर्काऊ arrayp->subDTypep()->width() > 1;
        }
        अचर AstStructDType* अचर structp = VN_CAST(dtypep, StructDType);
        फर्काऊ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    स्थिर तार्किक isSplittableType(अचर Ast_गाँठो_DType* dtypep) {
        यदि (isPacked(dtypep)) फर्काऊ v3Global.opt.fDecomposePacked();
        यदि (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            यदि (अचर AstStructDType* अचर structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                लागि (अचर AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    यदि (mp->valuep()) फर्काऊ असत्य;
                }
            }
            फर्काऊ v3Global.opt.fDecomposeUnpacked();
        }
        फर्काऊ असत्य;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    स्थिर Ast_गाँठो_DType* dtypeOf(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (अचर AstVarRef* अचर refp = VN_CAST(nodep, VarRef)) फर्काऊ refp->varp()->dtypep();
        फर्काऊ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    अचर std::vector<Component>& dtypeComponents(Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        यदि (compsr.empty()) {
            यदि (अचर Ast_गाँठो_Array_DType* अचर arrayp = VN_CAST(dtypep, गाँठो_Array_DType)) {
                अचर तार्किक packed = isPacked(dtypep);
                अचर VNumRange range = arrayp->declRange();
                अचर तार्किक rev = packed && range.ascending();  // Only for naming components
                Ast_गाँठो_DType* अचर subp = arrayp->subDTypep();
                लागि (पूर्णाङ्क i = 0; i < range.elements(); ++i) {
                    अचर पूर्णाङ्क lsb = packed ? i * subp->width() : 0;
                    अचर पूर्णाङ्क msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    अचर पूर्णाङ्क declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.थप_back({subp, nullptr, declIdx, lsb, msb});
                }
            } अन्यथा {
                अचर AstStructDType* अचर structp = VN_AS(dtypep, StructDType);
                अचर तार्किक packed = isPacked(dtypep);
                लागि (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    अचर पूर्णाङ्क lsb = packed ? mp->lsb() : 0;
                    अचर पूर्णाङ्क msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.थप_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        फर्काऊ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    माप_प्रकार componentIndex(Ast_गाँठो_DType* dtypep, पूर्णाङ्क bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        अचर std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        अचर स्वतः it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](अचर Component& comp, पूर्णाङ्क b) {  //
                                             फर्काऊ b > comp.msb;
                                         });
        फर्काऊ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, माप_प्रकार idx) {
        अचर std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        यदि (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        यदि (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        फर्काऊ childpr.get();
    }

    // Is 'nodep' cheap to clone
    स्थिर तार्किक isCheap(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
        // Constants are cheap
        यदि (VN_IS(nodep, Const)) फर्काऊ सत्य;
        // So are resets
        यदि (VN_IS(nodep, CReset)) फर्काऊ सत्य;
        // Variable references are cheap for the most part
        यदि (अचर AstVarRef* अचर refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            यदि (refp->varp()->isForced()) फर्काऊ असत्य;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            यदि (refp->varp()->isSc()) फर्काऊ असत्य;
            // Otherwise the path rooted here is cheap
            फर्काऊ सत्य;
        }
        // Bit selects only with a constant LSB
        यदि (अचर AstSel* अचर selp = VN_CAST(nodep, Sel)) {
            फर्काऊ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        यदि (अचर AstArraySel* अचर selp = VN_CAST(nodep, ArraySel)) {
            फर्काऊ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        यदि (अचर AstStructSel* अचर selp = VN_CAST(nodep, StructSel)) {  //
            फर्काऊ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        यदि (अचर AstExtend* अचर extp = VN_CAST(nodep, Extend)) {  //
            फर्काऊ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        फर्काऊ असत्य;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

वर्ग DecomposeRecord final : सार्वजनिक VNVisitorConst, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    अचर VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_गाँठो_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, अचर Ast_गाँठो* wherep, अचर वर्ण* reasonp) {
        // Only warn if user requested splitting
        यदि (!varp->attrSplitVar()) फर्काऊ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        यदि (isPacked(varp->dtypep())) फर्काऊ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, सत्य);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        यदि (!varp->user1()) {
            अचर तार्किक eligible = [&]() {
                // Check data type
                यदि (!isSplittableType(varp->dtypep())) फर्काऊ असत्य;
                // Check properties
                अचर वर्ण* अचर reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                यदि (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                फर्काऊ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        फर्काऊ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(अचर AstVarRef* refp) {
        AstVarScope* अचर vscp = refp->varScopep();
        यदि (!isEligible(vscp->varp())) फर्काऊ nullptr;
        Place& place = m_state.places(vscp);
        यदि (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.थप_back(&place);
        }
        फर्काऊ &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    स्थिर शून्य block(Place* placep, अचर Ast_गाँठो* wherep) {
        placep->splittable = असत्य;
        // Warn on the variable only, a component is split as far as possible, as requested
        यदि (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_गाँठो_अभिव्यक्ति* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        यदि (AstArraySel* अचर selp = VN_CAST(exprp, ArraySel)) {
            यदि (Place* अचर fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                अचर AstConst* अचर bitp = VN_CAST(selp->bitp(), Const);
                यदि (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* अचर childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = सत्य;
                    chain.थप_back({selp, childp, 0});
                    फर्काऊ childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            फर्काऊ nullptr;
        }
        // Struct selects correspond one to one with a component select
        यदि (AstStructSel* अचर selp = VN_CAST(exprp, StructSel)) {
            यदि (Place* अचर fromp = resolveChain(selp->fromp(), chain)) {
                यदि (!isUnpacked(fromp->dtypep)) फर्काऊ nullptr;
                AstStructDType* अचर structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                यदि (!structp->user1SetOnce()) {
                    अचर std::vector<Component>& compsr = dtypeComponents(structp);
                    लागि (माप_प्रकार i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                अचर Ast_गाँठो* अचर memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* अचर childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = सत्य;
                chain.थप_back({selp, childp, 0});
                फर्काऊ childp;
            }
            फर्काऊ nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        यदि (AstSel* अचर selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            यदि (!placep) {
                iterateConst(selp->lsbp());
                फर्काऊ nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            अचर AstConst* अचर lsbp = VN_CAST(selp->lsbp(), Const);
            यदि (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                फर्काऊ nullptr;
            }
            // Descend into the Place containing the bits, until exact
            पूर्णाङ्क lsb = lsbp->toSInt();
            पूर्णाङ्क msb = lsb + selp->widthConst() - 1;
            जबसम्म (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                यदि (!isPacked(placep->dtypep)) फर्काऊ nullptr;
                अचर माप_प्रकार idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                यदि (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    फर्काऊ nullptr;
                }
                placep->wantsSplit = सत्य;
                अचर Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.थप_back({selp, placep, lsb});
            }
            फर्काऊ placep;
        }
        // Base case
        यदि (AstVarRef* अचर refp = VN_CAST(exprp, VarRef)) फर्काऊ placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        फर्काऊ nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    शून्य referencedWhole(Ast_गाँठो_अभिव्यक्ति* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        यदि (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            फर्काऊ;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    शून्य visitSelect(Ast_गाँठो_अभिव्यक्ति* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* अचर placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        यदि (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        यदि (!chain.empty()) m_state.chains.थप_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    स्थिर तार्किक hasReversedRange(अचर Ast_गाँठो_DType* aDTypep, अचर Ast_गाँठो_DType* bDTypep) {
        अचर AstUnpackArrayDType* अचर aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        अचर AstUnpackArrayDType* अचर bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        यदि (!aArrayp || !bArrayp) फर्काऊ असत्य;
        यदि (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) फर्काऊ सत्य;
        फर्काऊ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    शून्य visitAssignment(Ast_गाँठो_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        अचर तार्किक reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        यदि (!m_assignp) फर्काऊ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp();
        Place* अचर lPlacep = lhsp->user1u().to<Place*>();
        Place* अचर rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        अचर तार्किक splittable = [&]() {
            // Assignment between two Places is splittable
            यदि (lPlacep && rPlacep) फर्काऊ सत्य;
            // If the LHS is a Place, the RHS must be a splittable expression
            यदि (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                यदि (isPacked(lPlacep->dtypep)) फर्काऊ सत्य;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                फर्काऊ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            यदि (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                यदि (isPacked(rPlacep->dtypep)) फर्काऊ सत्य;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                फर्काऊ isCheap(lhsp);
            }
            // Otherwise not splittable
            फर्काऊ असत्य;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        यदि (!splittable) {
            यदि (rPlacep) block(rPlacep, rhsp);
            यदि (lPlacep) block(lPlacep, lhsp);
            फर्काऊ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        यदि (lPlacep && rPlacep) {
            अचर पूर्णाङ्क width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.थप_back({rPlacep, 0, 0, width});
            rPlacep->copies.थप_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.थप_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_DType*) override {}  // No references in data types
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }
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
        यदि (Place* अचर placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* अचर varp = nodep->varp();
        यदि (!isEligible(varp)) फर्काऊ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        लागि (Place* अचर placep : m_state.rootps) {
            यदि (!isEligible(placep->vscp->varp())) placep->splittable = असत्य;
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

वर्ग DecomposeDecision final : सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    अचर VNUser1InUse m_user1InUse;

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
    स्थिर तार्किक canSplit(अचर Place* placep) {
        // Not if not splittable
        यदि (!placep->splittable) फर्काऊ असत्य;
        // Not if already split
        यदि (placep->split) फर्काऊ असत्य;
        // Original variable or its parent was already split
        फर्काऊ !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    शून्य enqueue(Place* placep) {
        यदि (placep->queued || !placep->wantsSplit || !canSplit(placep)) फर्काऊ;
        placep->queued = सत्य;
        m_worklist.थप_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    शून्य createComponents(Place* placep) {
        AstVarScope* अचर vscp = placep->vscp;
        AstVar* अचर varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        यदि (varps.empty()) {
            अचर तार्किक packed = isPacked(varp->dtypep());
            यदि (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } अन्यथा {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_पङ्क्ति* अचर flp = varp->fileline();
            अचर VVarType varType = varp->varType();
            अचर std::string name = varp->name();
            लागि (अचर Component& comp : dtypeComponents(varp->dtypep())) {
                अचर std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_गाँठो::encode_सङ्ख्या(comp.declIdx) + "__KET__";
                AstVar* अचर newp = नूतन AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.थप_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* अचर scopep = vscp->scopep();
        File_पङ्क्ति* अचर flp = vscp->fileline();
        लागि (माप_प्रकार i = 0; i < varps.size(); ++i) {
            AstVarScope* अचर newp = नूतन AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    शून्य addCopy(Place* placep, अचर Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        यदि (!placep->splittable) फर्काऊ;
        // Add the copy to the Place
        placep->copies.थप_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        यदि (!isPacked(placep->dtypep)) फर्काऊ;
        यदि (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = सत्य;
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
    शून्य splitCopy(Place* placep, अचर Copy& copy) {
        Place* अचर otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        यदि (isPacked(placep->dtypep)) {
            अचर std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            अचर पूर्णाङ्क lsb = copy.lsb;
            अचर पूर्णाङ्क msb = copy.lsb + copy.width - 1;
            अचर माप_प्रकार first = componentIndex(placep->dtypep, lsb);
            लागि (माप_प्रकार i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                अचर Component& comp = comps[i];
                अचर पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                अचर पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            फर्काऊ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = सत्य;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        अचर माप_प्रकार size = dtypeComponents(placep->dtypep).size();
        लागि (माप_प्रकार i = 0; i < size; ++i) {
            Place* अचर ap = componentOf(placep, i);
            यदि (!ap->splittable) जारी;
            Place* अचर bp = componentOf(otherp, i);
            यदि (!bp->splittable) जारी;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    शून्य splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = सत्य;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        अचर माप_प्रकार size = dtypeComponents(placep->dtypep).size();
        लागि (माप_प्रकार i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        लागि (अचर Copy& copy : placep->copies) {
            यदि (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        लागि (Place* अचर placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        जबसम्म (!m_worklist.empty()) {
            Place* अचर placep = m_worklist.back();
            m_worklist.झिक_back();
            placep->queued = असत्य;
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

वर्ग DecomposeRewrite final : सार्वजनिक VNDeleter, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    अचर VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    स्थिर AstVarRef* newRef(File_पङ्क्ति* flp, अचर Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        फर्काऊ नूतन AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    शून्य rewriteChain(अचर std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        अचर Select* selectp = nullptr;
        लागि (अचर Select& select : chain) {
            यदि (select.placep->parentp->split) selectp = &select;
        }
        यदि (!selectp) फर्काऊ;
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        अचर VAccess access = [&]() {
            अचर Ast_गाँठो_अभिव्यक्ति* rootp = chain.front().exprp;
            जबसम्म (सत्य) {
                यदि (अचर AstArraySel* अचर selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } अन्यथा यदि (अचर AstStructSel* अचर selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } अन्यथा यदि (अचर AstSel* अचर selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } अन्यथा {
                    फर्काऊ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_पङ्क्ति* अचर flp = exprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* newp = newRef(flp, selectp->placep, access);
        यदि (अचर AstSel* अचर selp = VN_CAST(exprp, Sel)) {
            अचर पूर्णाङ्क width = selp->widthConst();
            यदि (width != newp->width()) newp = नूतन AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_गाँठो_अभिव्यक्ति* newSelect(Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtypep, माप_प्रकार idx) {
        File_पङ्क्ति* अचर flp = fromp->fileline();
        अचर Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        यदि (isPacked(dtypep)) {
            AstSel* अचर selp = नूतन AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            फर्काऊ selp;
        }
        // If unpacked array, it's an ArraySel
        यदि (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            अचर पूर्णाङ्क i = static_cast<पूर्णाङ्क>(idx);
            UASSERT_OBJ(static_cast<माप_प्रकार>(i) == idx, fromp, "Index overflow");
            फर्काऊ नूतन AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* अचर selp = नूतन AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        फर्काऊ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    स्थिर std::vector<Ast_गाँठो_अभिव्यक्ति*> concatTerms(Ast_गाँठो_अभिव्यक्ति* nodep) {
        std::vector<Ast_गाँठो_अभिव्यक्ति*> termps;
        std::vector<Ast_गाँठो_अभिव्यक्ति*> थाक{nodep};
        जबसम्म (!थाक.empty()) {
            Ast_गाँठो_अभिव्यक्ति* अचर exprp = थाक.back();
            थाक.झिक_back();
            यदि (AstConcat* अचर concatp = VN_CAST(exprp, Concat)) {
                थाक.थप_back(concatp->lhsp());
                थाक.थप_back(concatp->rhsp());
                जारी;
            }
            यदि (AstReplicate* अचर repp = VN_CAST(exprp, Replicate)) {
                अचर AstConst* अचर countp = VN_AS(repp->countp(), Const);
                लागि (uint32_t i = 0; i < countp->toUInt(); ++i) थाक.थप_back(repp->srcp());
                जारी;
            }
            termps.थप_back(exprp);
        }
        फर्काऊ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    स्थिर तार्किक isSliceable(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (अचर AstSel* अचर selp = VN_CAST(nodep, Sel)) {
            फर्काऊ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        यदि (अचर AstConcat* अचर catp = VN_CAST(nodep, Concat)) {
            फर्काऊ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        यदि (अचर AstCond* अचर condp = VN_CAST(nodep, Cond)) {
            फर्काऊ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        यदि (अचर AstAnd* अचर andp = VN_CAST(nodep, And)) {
            फर्काऊ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        यदि (अचर AstOr* अचर orp = VN_CAST(nodep, Or)) {
            फर्काऊ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        यदि (अचर AstXor* अचर xorp = VN_CAST(nodep, Xor)) {
            फर्काऊ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        यदि (अचर AstExtend* अचर extp = VN_CAST(nodep, Extend)) {
            फर्काऊ isSliceable(extp->lhsp());
        }
        यदि (अचर AstNot* अचर notp = VN_CAST(nodep, Not)) {  //
            फर्काऊ isSliceable(notp->lhsp());
        }
        यदि (अचर AstReplicate* अचर repp = VN_CAST(nodep, Replicate)) {
            फर्काऊ isSliceable(repp->srcp());
        }
        फर्काऊ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    स्थिर Ast_गाँठो_अभिव्यक्ति* newSlice(Ast_गाँठो_अभिव्यक्ति* nodep, पूर्णाङ्क lsb, पूर्णाङ्क width) {
        File_पङ्क्ति* अचर flp = nodep->fileline();
        // A constant, as a new constant of the bits
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) {
            V3_सङ्ख्या अङ्क{nodep, width};
            अङ्क.opSel(constp->अङ्क(), lsb + width - 1, lsb);
            फर्काऊ नूतन AstConst{flp, अङ्क};
        }
        // A constant bit select, from the bits of what it selects from
        यदि (AstSel* अचर selp = VN_CAST(nodep, Sel)) {
            यदि (अचर AstConst* अचर lsbp = VN_CAST(selp->lsbp(), Const)) {
                फर्काऊ newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        यदि (AstConcat* अचर catp = VN_CAST(nodep, Concat)) {
            अचर पूर्णाङ्क rWidth = catp->rhsp()->width();
            अचर पूर्णाङ्क msb = lsb + width - 1;
            यदि (msb < rWidth) फर्काऊ newSlice(catp->rhsp(), lsb, width);
            यदि (lsb >= rWidth) फर्काऊ newSlice(catp->lhsp(), lsb - rWidth, width);
            फर्काऊ नूतन AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        यदि (AstCond* अचर condp = VN_CAST(nodep, Cond)) {
            फर्काऊ नूतन AstCond{flp, condp->condp()->clone_रूख_Pure(असत्य),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        यदि (AstAnd* अचर andp = VN_CAST(nodep, And)) {
            फर्काऊ नूतन AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        यदि (AstOr* अचर orp = VN_CAST(nodep, Or)) {
            फर्काऊ नूतन AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        यदि (AstXor* अचर xorp = VN_CAST(nodep, Xor)) {
            फर्काऊ नूतन AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        यदि (AstExtend* अचर extp = VN_CAST(nodep, Extend)) {
            अचर पूर्णाङ्क srcWidth = extp->lhsp()->width();
            अचर पूर्णाङ्क msb = lsb + width - 1;
            यदि (msb < srcWidth) फर्काऊ newSlice(extp->lhsp(), lsb, width);
            यदि (lsb >= srcWidth) फर्काऊ नूतन AstConst{flp, AstConst::Widthed_मान{}, width, 0};
            फर्काऊ नूतन AstConcat{
                flp, नूतन AstConst{flp, AstConst::Widthed_मान{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        यदि (AstNot* अचर notp = VN_CAST(nodep, Not)) {
            फर्काऊ नूतन AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        यदि (AstReplicate* अचर repp = VN_CAST(nodep, Replicate)) {
            Ast_गाँठो_अभिव्यक्ति* अचर srcp = repp->srcp();
            अचर पूर्णाङ्क srcWidth = srcp->width();
            अचर पूर्णाङ्क msb = lsb + width - 1;
            Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
            लागि (पूर्णाङ्क copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                अचर पूर्णाङ्क partLsb = std::max(lsb, copyLsb) - copyLsb;
                अचर पूर्णाङ्क partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_गाँठो_अभिव्यक्ति* अचर bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
            }
            फर्काऊ resultp;
        }
        Ast_गाँठो_अभिव्यक्ति* अचर clonep = nodep->clone_रूख_Pure(असत्य);
        यदि (lsb == 0 && width == nodep->width()) फर्काऊ clonep;
        फर्काऊ नूतन AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_गाँठो_अभिव्यक्ति*> newAssignRhsps(Ast_गाँठो_अभिव्यक्ति* nodep, Ast_गाँठो_DType* dtypep) {
        अचर std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_गाँठो_अभिव्यक्ति*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        यदि (AstCReset* अचर cresetp = VN_CAST(nodep, CReset)) {
            लागि (अचर Component& comp : compsr) {
                AstCReset* अचर newp = cresetp->clone_रूख(असत्य);
                newp->dtypep(comp.dtypep);
                valueps.थप_back(newp);
            }
            फर्काऊ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        यदि (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            पूर्णाङ्क lsb = 0;  // LSB of the current term
            लागि (Ast_गाँठो_अभिव्यक्ति* अचर termp : concatTerms(nodep)) {
                अचर पूर्णाङ्क msb = lsb + termp->width() - 1;
                अचर माप_प्रकार first = componentIndex(dtypep, lsb);
                लागि (माप_प्रकार i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    अचर Component& comp = compsr[i];
                    अचर पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                    अचर पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                    File_पङ्क्ति* अचर flp = termp->fileline();
                    Ast_गाँठो_अभिव्यक्ति* अचर sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? नूतन AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            फर्काऊ valueps;
        }

        // Otherwise unpacked, so select each component
        लागि (माप_प्रकार i = 0; i < compsr.size(); ++i) {
            valueps.थप_back(newSelect(nodep->clone_रूख(असत्य), dtypep, i));
        }
        फर्काऊ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_गाँठो_अभिव्यक्ति* newBits(File_पङ्क्ति* flp, अचर Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        Ast_गाँठो_DType* अचर dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        यदि (!placep->split) {
            AstVarRef* अचर refp = newRef(flp, placep, VAccess::READ);
            यदि (lsb == 0 && msb == dtypep->width() - 1) फर्काऊ refp;
            फर्काऊ नूतन AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        अचर std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
        अचर माप_प्रकार first = componentIndex(dtypep, lsb);
        लागि (माप_प्रकार i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            अचर Component& comp = comps[i];
            अचर पूर्णाङ्क partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            अचर पूर्णाङ्क partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* अचर childp = placep->childrenp.at(i).get();
            Ast_गाँठो_अभिव्यक्ति* अचर bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        फर्काऊ resultp;
    }

    स्थिर शून्य addNewAssign(Ast_गाँठो_Assign* origp, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    शून्य expandSS(Ast_गाँठो_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_पङ्क्ति* अचर flp = origp->fileline();
        अचर std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        यदि (isPacked(rPlacep->dtypep)) {
            लागि (माप_प्रकार i = 0; i < compsr.size(); ++i) {
                Place* अचर lChildp = lPlacep->childrenp.at(i).get();
                Ast_गाँठो_अभिव्यक्ति* अचर rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } अन्यथा {
            लागि (माप_प्रकार i = 0; i < compsr.size(); ++i) {
                Place* अचर lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    शून्य expandSN(Ast_गाँठो_Assign* origp, Place* lPlacep, Ast_गाँठो_अभिव्यक्ति* rhsp) {
        अचर std::vector<Ast_गाँठो_अभिव्यक्ति*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        लागि (माप_प्रकार i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(थप_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    शून्य expandNS(Ast_गाँठो_Assign* origp, Ast_गाँठो_अभिव्यक्ति* lhsp, Place* rPlacep) {
        File_पङ्क्ति* अचर flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        यदि (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            फर्काऊ;
        }
        // Unpacked of the same shape, component-wise
        Ast_गाँठो_DType* अचर dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        अचर तार्किक readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            अचर VNUser2InUse user2InUse;
            फर्काऊ lhsp->exists([](अचर AstVarRef* refp) {
                AstVarScope* अचर vscp = refp->varScopep();
                यदि (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                यदि (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                फर्काऊ vscp->user2() == 3;
            });
        }();
        यदि (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* अचर scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* अचर vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, नूतन AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, नूतन AstVarRef{flp, vscp, VAccess::READ});
            फर्काऊ;
        }
        अचर माप_प्रकार size = dtypeComponents(dtypep).size();
        लागि (माप_प्रकार i = 0; i < size; ++i) {
            Ast_गाँठो_अभिव्यक्ति* अचर newLhsp = newSelect(lhsp->clone_रूख(असत्य), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(थप_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    शून्य expand(Ast_गाँठो_Assign* origp,  //
                Place* lPlacep, Ast_गाँठो_अभिव्यक्ति* lhsp,  //
                Place* rPlacep, Ast_गाँठो_अभिव्यक्ति* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_पङ्क्ति* अचर flp = origp->fileline();
        // A Place not split is referenced
        यदि (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        यदि (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        यदि (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } अन्यथा यदि (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } अन्यथा यदि (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } अन्यथा {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    तार्किक isSplitBetween(अचर Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        जबसम्म (placep->split) {
            अचर माप_प्रकार idx = componentIndex(placep->dtypep, lsb);
            यदि (idx != componentIndex(placep->dtypep, msb)) फर्काऊ सत्य;
            अचर Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        फर्काऊ असत्य;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    शून्य hoistTerms(Ast_गाँठो_Assign* origp, अचर Place* lPlacep) {
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = origp->rhsp();
        अचर std::vector<Ast_गाँठो_अभिव्यक्ति*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        अचर VNUser2InUse user2InUse;
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* अचर scopep = lPlacep->rootp->vscp->scopep();
        पूर्णाङ्क lsb = 0;  // LSB of the current term
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर termp : termps) {
            अचर पूर्णाङ्क tLsb = lsb;
            अचर पूर्णाङ्क tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            यदि (!termp->user2()) जारी;
            // Cheap to select from
            यदि (isCheap(termp)) {
                termp->user2(0);
                जारी;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            यदि (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                जारी;
            }
            // Bits can be selected without evaluating it more than once
            यदि (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                जारी;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_पङ्क्ति* अचर flp = termp->fileline();
            AstVarScope* अचर vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* अचर tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    स्थिर शून्य markComponents(अचर Place* placep) {
        लागि (अचर std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(सत्य);
            यदि (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    शून्य rewriteAssignment(अचर Assignment& assignment) {
        Place* अचर lPlacep = assignment.lPlacep;
        Place* अचर rPlacep = assignment.rPlacep;
        अचर तार्किक lSplit = lPlacep && lPlacep->split;
        अचर तार्किक rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        यदि (!lSplit && !rSplit) फर्काऊ;

        Ast_गाँठो_Assign* अचर origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        यदि (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            अचर VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = origp->rhsp();
            यदि (rhsp->exists([](अचर AstVarRef* refp) { फर्काऊ refp->varScopep()->user2(); })) {
                File_पङ्क्ति* अचर flp = rhsp->fileline();
                AstScope* अचर scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* अचर vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* अचर tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(नूतन AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        यदि (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = origp->lhsp()->unlinkFrBack();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        यदि (lSplit) VL_DO_DANGLING(थप_Deletep(lhsp), lhsp);
        यदि (rSplit) VL_DO_DANGLING(थप_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(थप_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        यदि (!scopep->user1p()) scopep->user1p(scopep->comboActivep(सत्य));
        फर्काऊ VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    शून्य driveFromComponents(Place* placep, AstActive* activep) {
        यदि (!placep->split) फर्काऊ;
        AstVarScope* अचर vscp = placep->vscp;
        File_पङ्क्ति* अचर flp = vscp->fileline();
        लागि (माप_प्रकार i = 0; i < placep->childrenp.size(); ++i) {
            Place* अचर childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_गाँठो_अभिव्यक्ति* अचर lhsRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(नूतन AstAlways{नूतन AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        लागि (अचर std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        लागि (अचर Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        लागि (Place* अचर placep : m_state.rootps) {
            यदि (!placep->split) जारी;
            अचर AstVarScope* अचर vscp = placep->vscp;
            // Only traced ones at this point
            यदि (!(vscp->varp()->isTrace() && vscp->isTrace())) जारी;
            AstActive* अचर activep = comboActive(vscp->scopep());
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
    V3Global::dump_Check_Global_रूख("decompose", 0, dump_रूख_Either_Level() >= 3);
}
