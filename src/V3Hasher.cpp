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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Hasher.h"

#അന്തര്ഭാവ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

വർഗ്ഗം HasherVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    സ്ഥിരം തർക്കം m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_കണ്ണി*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    ശൂന്യം guardRecursion(Ast_കണ്ണി* സ്ഥിരം nodep, std::function<ശൂന്യം()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        എങ്കിൽ (!m_cacheInUser4) {
            എങ്കിൽ (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                തിരികെ;
            }
            m_visited.തിരുകുക(nodep);
        }
        f();
    }

    V3Hash hash_കണ്ണി_And_Iterate(Ast_കണ്ണി* nodep, തർക്കം hashDType, തർക്കം hashChildren,
                              std::function<ശൂന്യം()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        എങ്കിൽ (m_cacheInUser4 && nodep->user4()) {
            തിരികെ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            എങ്കിൽ (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            എങ്കിൽ (hashChildren) iterateChildrenConst(nodep);  // Children
            എങ്കിൽ (m_cacheInUser4) nodep->user4(m_hash.മൂല്യം());
            തിരികെ m_hash;
        }
    }

    // VISITORS

    constexpr സ്ഥിതം തർക്കം HASH_DTYPE = സത്യം;
    constexpr സ്ഥിതം തർക്കം HASH_CHILDREN = സത്യം;

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

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
#എങ്കിൽ VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    ശൂന്യം visit(Ast_കണ്ണി_Array_DType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ഇടത്();
            m_hash += nodep->വലത്();
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, അസത്യം, [ഇത്, nodep]() {  //
            m_hash += nodep->unique_അക്കം();
        });
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ശൂന്യം visit(AstMemberDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->unique_അക്കം();
        });
    }
    ശൂന്യം visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    ശൂന്യം visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstDynArrayDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstBasicDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ഇടത്();
            m_hash += nodep->nrange().വലത്();
        });
    }
    ശൂന്യം visit(AstCDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstConstDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    ശൂന്യം visit(AstQueueDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    ശൂന്യം visit(AstStreamDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstVoidDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstEnumDType* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, അസത്യം, [ഇത്, nodep]() {  //
            m_hash += nodep->unique_അക്കം();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstSel* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    ശൂന്യം visit(AstConst* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->അക്കം().toHash();
        });
    }
    ശൂന്യം visit(AstNullCheck* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstCCast* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            എങ്കിൽ (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } അല്ലെങ്കിൽ {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    ശൂന്യം visit(AstVarXRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ശൂന്യം visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ശൂന്യം visit(AstTriggered* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            വേണ്ടി (സ്ഥിരം uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ശൂന്യം visit(AstCLocalScope* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstCoverInc* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ശൂന്യം visit(AstDisplay* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ശൂന്യം visit(AstMonitorOff* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, അസത്യം, []() {});
    }
    ശൂന്യം visit(AstTraceInc* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Or_Assert* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    ശൂന്യം visit(Ast_കണ്ണി_Range* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, അസത്യം, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_Pre_Sel* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstClassExtends* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstForeachHeader* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstDefParam* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstArg* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstParseRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ശൂന്യം visit(AstElabDisplay* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons_Item* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ അസത്യം,
                                     [ഇത്, nodep, dtypep]() {
                                         സ്ഥിരം uint32_t size = dtypep->elementsConst();
                                         വേണ്ടി (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    ശൂന്യം visit(AstPragma* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    ശൂന്യം visit(AstAttrOf* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_File* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [ഇത്, nodep]() {  //
            m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                എങ്കിൽ (m_cacheInUser4) nodep->user4(V3Hash{1}.മൂല്യം());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    ശൂന്യം visit(AstVar* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ശൂന്യം visit(AstScope* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, അസത്യം, [ഇത്, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    ശൂന്യം visit(AstEnumItem* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstTypedefFwd* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstActive* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ശൂന്യം visit(AstCell* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    ശൂന്യം visit(AstCellInline* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ഇത്, nodep]() { m_hash += nodep->name(); });
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        guardRecursion(nodep, [ഇത്, nodep]() {  //
            m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                എങ്കിൽ (m_cacheInUser4) nodep->user4(V3Hash{1}.മൂല്യം());
            });
        });
    }
    ശൂന്യം visit(AstModport* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstModportVarRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    ശൂന്യം visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ശൂന്യം visit(AstFork* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    ശൂന്യം visit(AstPin* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ഇത്, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_അക്കം();
        });
    }
    ശൂന്യം visit(AstText* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    ശൂന്യം visit(AstTextBlock* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, [ഇത്, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, അസത്യം, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ശൂന്യം visit(AstWith* nodep) override {
        m_hash += hash_കണ്ണി_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

പൊതു:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_കണ്ണി* nodep)
        : m_cacheInUser4{സത്യം} {
        iterateConst(nodep);
    }
    വർഗ്ഗം Uncached {};
    HasherVisitor(സ്ഥിരം Ast_കണ്ണി* nodep, Uncached)
        : m_cacheInUser4{അസത്യം} {
        iterateConst(const_cast<Ast_കണ്ണി*>(nodep));
    }
    V3Hash finalHash() സ്ഥിരം { തിരികെ m_hash; }
    ~HasherVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_കണ്ണി* nodep) സ്ഥിരം {
    എങ്കിൽ (!nodep->user4()) HasherVisitor{nodep};
    തിരികെ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_കണ്ണി* nodep) സ്ഥിരം {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    തിരികെ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(സ്ഥിരം Ast_കണ്ണി* nodep) {
    സ്ഥിരം HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    തിരികെ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(സ്ഥിരം Ast_കണ്ണി& കണ്ണി) {
    തിരികെ static_cast<size_t>(V3Hasher::uncachedHash(&കണ്ണി).മൂല്യം());
}
