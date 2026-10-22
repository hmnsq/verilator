// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#ifndef VERILATOR_V3HIERBLOCK_H_
#define VERILATOR_V3HIERBLOCK_H_

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

class AstNetlist;
class Ast_ଗଣ୍ଠି_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : ସର୍ବସାଧାରଣ V3Graph {
ସର୍ବସାଧାରଣ:
    V3HierGraph() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3HierGraph() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ସ୍ଥିର VStringList sourceFiles(ଅଚଳ string& ଶୀର୍ଷ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    ଶୂନ୍ୟ writeCommandArgsFiles(bool forMkJson) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ writeParametersFiles() ଅଚଳ VL_MT_DISABLED;
    ସ୍ଥିର string ଶୀର୍ଷ_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    ଅଚଳ AstModule* ଅଚଳ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ଅଚଳ std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ଅଚଳ std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    ସ୍ଥିର StrGParams stringifyParams(ଅଚଳ std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, ଅଚଳ AstModule* modp, ଅଚଳ std::vector<AstVar*>& params,
                ଅଚଳ std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    ଅଚଳ AstModule* modp() ଅଚଳ { ଫେରନ୍ତୁ m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ସ୍ଥିର bool stringParamPassable(ଅଚଳ string& ମୂଲ୍ୟ) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) ଅଚଳ VL_MT_DISABLED;
    VStringList hierBlockArgs() ଅଚଳ VL_MT_DISABLED;
    string hierPrefix() ଅଚଳ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ଅଚଳ ଅକ୍ଷର* prefix,
                            ଅଚଳ ଅକ୍ଷର* suffix) ଅଚଳ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ଅଚଳ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ଅଚଳ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ଅଚଳ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ଅଚଳ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ଅଚଳ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    ଶୂନ୍ୟ writeCommandArgsFile(bool forMkJson) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ writeParametersFile() ଅଚଳ VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) ଅଚଳ VL_MT_DISABLED;
    string typeParametersFilename() ଅଚଳ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ଅଚଳ override { ଫେରନ୍ତୁ m_modp->prettyNameQ(); }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
