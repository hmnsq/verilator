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

#شامل "V3PchAstMT.h"

#شامل "V3Hasher.h"

#شامل <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

رده HasherVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ثابت منطقی m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_گره*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    تهی guardRecursion(Ast_گره* ثابت nodep, std::function<تهی()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        اگر (!m_cacheInUser4) {
            اگر (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                بازگشت;
            }
            m_visited.درج(nodep);
        }
        f();
    }

    V3Hash hash_گره_And_Iterate(Ast_گره* nodep, منطقی hashDType, منطقی hashChildren,
                              std::function<تهی()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        اگر (m_cacheInUser4 && nodep->user4()) {
            بازگشت V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وگرنه {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            اگر (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            اگر (hashChildren) iterateChildrenConst(nodep);  // Children
            اگر (m_cacheInUser4) nodep->user4(m_hash.مقدار());
            بازگشت m_hash;
        }
    }

    // VISITORS

    constexpr ایستا منطقی HASH_DTYPE = درست;
    constexpr ایستا منطقی HASH_CHILDREN = درست;

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

    تهی visit(Ast_گره* nodep) override {
#اگر VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    تهی visit(Ast_گره_Array_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->چپ();
            m_hash += nodep->راست();
        });
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, نادرست, [این, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }
    تهی visit(AstParamTypeDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    تهی visit(AstMemberDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }
    تهی visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    تهی visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstDynArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstBasicDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().چپ();
            m_hash += nodep->nrange().راست();
        });
    }
    تهی visit(AstCDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstConstDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstClassRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    تهی visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    تهی visit(AstQueueDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    تهی visit(AstRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    تهی visit(AstStreamDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(AstVoidDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(AstEnumDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, نادرست, [این, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    تهی visit(Ast_گره_عبارت* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstSel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    تهی visit(AstConst* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->رقم().toHash();
        });
    }
    تهی visit(AstNullCheck* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstCCast* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    تهی visit(AstVarRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            اگر (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وگرنه {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    تهی visit(AstVarXRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    تهی visit(AstMemberSel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstFScanF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    تهی visit(AstSScanF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    تهی visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    تهی visit(AstTriggered* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            برای (ثابت uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    تهی visit(Ast_گره_Stmt* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    تهی visit(AstCaseItem* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    تهی visit(AstCMethodHard* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    تهی visit(AstCAwait* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    تهی visit(AstCLocalScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstCoverInc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    تهی visit(AstDisplay* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    تهی visit(AstMonitorOff* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    تهی visit(AstJumpGo* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, نادرست, []() {});
    }
    تهی visit(AstTraceInc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    تهی visit(Ast_گره_Cover_Or_Assert* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    تهی visit(Ast_گره_Range* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(Ast_گره_Module* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, نادرست, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(Ast_گره_Pre_Sel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstClassExtends* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstForeachHeader* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstDefParam* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstArg* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstParseRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    تهی visit(AstSenItem* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstSFormatF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    تهی visit(AstElabDisplay* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    تهی visit(Ast_گره_Cons_Item* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ نادرست,
                                     [این, nodep, dtypep]() {
                                         ثابت uint32_t size = dtypep->elementsConst();
                                         برای (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    تهی visit(AstPragma* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    تهی visit(AstAttrOf* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    تهی visit(Ast_گره_File* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [این, nodep]() {  //
            m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                اگر (m_cacheInUser4) nodep->user4(V3Hash{1}.مقدار());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    تهی visit(AstVar* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    تهی visit(AstScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, نادرست, [این, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    تهی visit(AstVarScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    تهی visit(AstEnumItem* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstTypedef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstTypedefFwd* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstActive* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    تهی visit(AstCell* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    تهی visit(AstCellInline* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [این, nodep]() { m_hash += nodep->name(); });
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        guardRecursion(nodep, [این, nodep]() {  //
            m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                اگر (m_cacheInUser4) nodep->user4(V3Hash{1}.مقدار());
            });
        });
    }
    تهی visit(AstModport* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstModportVarRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    تهی visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstBegin* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    تهی visit(AstFork* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    تهی visit(AstPin* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [این, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_رقم();
        });
    }
    تهی visit(AstText* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    تهی visit(AstTextBlock* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, [این, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    تهی visit(AstCStmt* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(AstCStmtUser* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, نادرست, HASH_CHILDREN, []() {});
    }
    تهی visit(AstCExpr* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstCExprUser* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    تهی visit(AstWith* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عمومی:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_گره* nodep)
        : m_cacheInUser4{درست} {
        iterateConst(nodep);
    }
    رده Uncached {};
    HasherVisitor(ثابت Ast_گره* nodep, Uncached)
        : m_cacheInUser4{نادرست} {
        iterateConst(const_cast<Ast_گره*>(nodep));
    }
    V3Hash finalHash() ثابت { بازگشت m_hash; }
    ~HasherVisitor() override = پیشفرض;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_گره* nodep) ثابت {
    اگر (!nodep->user4()) HasherVisitor{nodep};
    بازگشت V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_گره* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    بازگشت V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_گره* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    بازگشت visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(ثابت Ast_گره& گره) {
    بازگشت static_cast<size_t>(V3Hasher::uncachedHash(&گره).مقدار());
}
