// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode hash computation
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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"

#ଅନ୍ତର୍ଭାଵ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

ଶ୍ରେଣୀ HasherVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ଅଚଳ ତାର୍କିକ m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_ଗଣ୍ଠି*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    ଶୂନ୍ୟ guardRecursion(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, std::function<ଶୂନ୍ୟ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        ଯଦି (!m_cacheInUser4) {
            ଯଦି (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                ଫେରନ୍ତୁ;
            }
            m_visited.ଭର୍ତ୍ତି(nodep);
        }
        f();
    }

    V3Hash hash_ଗଣ୍ଠି_And_Iterate(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ hashDType, ତାର୍କିକ hashChildren,
                              std::function<ଶୂନ୍ୟ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        ଯଦି (m_cacheInUser4 && nodep->user4()) {
            ଫେରନ୍ତୁ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            ଯଦି (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            ଯଦି (hashChildren) iterateChildrenConst(nodep);  // Children
            ଯଦି (m_cacheInUser4) nodep->user4(m_hash.ମୂଲ୍ୟ());
            ଫେରନ୍ତୁ m_hash;
        }
    }

    // VISITORS

    constexpr ସ୍ଥିର ତାର୍କିକ HASH_DTYPE = ସତ୍ୟ;
    constexpr ସ୍ଥିର ତାର୍କିକ HASH_CHILDREN = ସତ୍ୟ;

    // Each visitor below contributes to the hash any node-specific content
    // that is not dependent on either of the following, as these are
    // included by default by hashNode:
    // - Node type (as given by AstNode::type().hash())
    // - Node dtype (unless !hashDType)
    // - child nodes (unless !hashChildren)
    //
    // The hash must be stable, which means in particular it cannot rely on
    // pointer values, or any other value that might differ between separate
    // invocations of Verilator over the same design.
    //
    // Note there is a circularity problem where some child nodes can back
    // to their ancestral nodes via member pointers, which can lead to an
    // infinite traversal. To break this, nodes that are subject to such
    // referencing and represent code which can reasonably be assumed not to
    // be equivalent to any other code, are hashed either by name (e.g.:
    // AstNodeModule), or by unique identifier (e.g.: AstNodeUOrStructDType).

    //------------------------------------------------------------
    // AstNode - Warns to help find missing cases

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
#ଯଦି VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Array_DType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ବାମ();
            m_hash += nodep->ଡାହାଣ();
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, [ଏହି, nodep]() {  //
            m_hash += nodep->unique_ଅଙ୍କ();
        });
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ଶୂନ୍ୟ visit(AstMemberDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->unique_ଅଙ୍କ();
        });
    }
    ଶୂନ୍ୟ visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    ଶୂନ୍ୟ visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstDynArrayDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ବାମ();
            m_hash += nodep->nrange().ଡାହାଣ();
        });
    }
    ଶୂନ୍ୟ visit(AstCDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstConstDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    ଶୂନ୍ୟ visit(AstQueueDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    ଶୂନ୍ୟ visit(AstStreamDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstVoidDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, [ଏହି, nodep]() {  //
            m_hash += nodep->unique_ଅଙ୍କ();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->ଅଙ୍କ().toHash();
        });
    }
    ଶୂନ୍ୟ visit(AstNullCheck* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstCCast* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            ଯଦି (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } ଅନ୍ୟଥା {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ଶୂନ୍ୟ visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ଶୂନ୍ୟ visit(AstTriggered* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            ପାଇଁ (ଅଚଳ uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ଶୂନ୍ୟ visit(AstCLocalScope* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstCoverInc* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ଶୂନ୍ୟ visit(AstMonitorOff* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, ମିଥ୍ୟା, []() {});
    }
    ଶୂନ୍ୟ visit(AstTraceInc* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Range* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, ମିଥ୍ୟା, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Pre_Sel* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstClassExtends* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstForeachHeader* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstDefParam* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstArg* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstParseRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ଶୂନ୍ୟ visit(AstElabDisplay* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons_Item* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ ମିଥ୍ୟା,
                                     [ଏହି, nodep, dtypep]() {
                                         ଅଚଳ uint32_t size = dtypep->elementsConst();
                                         ପାଇଁ (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    ଶୂନ୍ୟ visit(AstPragma* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_File* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [ଏହି, nodep]() {  //
            m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                ଯଦି (m_cacheInUser4) nodep->user4(V3Hash{1}.ମୂଲ୍ୟ());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, ମିଥ୍ୟା, [ଏହି, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    ଶୂନ୍ୟ visit(AstEnumItem* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstTypedefFwd* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    ଶୂନ୍ୟ visit(AstCellInline* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ଏହି, nodep]() { m_hash += nodep->name(); });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        guardRecursion(nodep, [ଏହି, nodep]() {  //
            m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                ଯଦି (m_cacheInUser4) nodep->user4(V3Hash{1}.ମୂଲ୍ୟ());
            });
        });
    }
    ଶୂନ୍ୟ visit(AstModport* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstModportVarRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    ଶୂନ୍ୟ visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ଏହି, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_ଅଙ୍କ();
        });
    }
    ଶୂନ୍ୟ visit(AstText* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ଶୂନ୍ୟ visit(AstTextBlock* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, [ଏହି, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, ମିଥ୍ୟା, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        m_hash += hash_ଗଣ୍ଠି_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_ଗଣ୍ଠି* nodep)
        : m_cacheInUser4{ସତ୍ୟ} {
        iterateConst(nodep);
    }
    ଶ୍ରେଣୀ Uncached {};
    HasherVisitor(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, Uncached)
        : m_cacheInUser4{ମିଥ୍ୟା} {
        iterateConst(const_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }
    V3Hash finalHash() ଅଚଳ { ଫେରନ୍ତୁ m_hash; }
    ~HasherVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
    ଯଦି (!nodep->user4()) HasherVisitor{nodep};
    ଫେରନ୍ତୁ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    ଫେରନ୍ତୁ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଅଚଳ HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    ଫେରନ୍ତୁ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(ଅଚଳ Ast_ଗଣ୍ଠି& ଗଣ୍ଠି) {
    ଫେରନ୍ତୁ static_cast<size_t>(V3Hasher::uncachedHash(&ଗଣ୍ଠି).ମୂଲ୍ୟ());
}
