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

#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Graph.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>
#অন্তর্ভাৱ <utility>
#অন্তর্ভাৱ <vector>

class AstNetlist;
class Ast_গ্ৰন্থি_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : সার্ৱজনিক V3Graph {
সার্ৱজনিক:
    V3HierGraph() = স্ৱভাৱ;
    ~V3HierGraph() = স্ৱভাৱ;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    স্থির VStringList sourceFiles(ধ্রুৱ string& শীৰ্ষ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    শূন্য writeCommandArgsFiles(bool forMkJson) ধ্রুৱ VL_MT_DISABLED;
    শূন্য writeParametersFiles() ধ্রুৱ VL_MT_DISABLED;
    স্থির string শীৰ্ষ_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    ধ্রুৱ AstModule* ধ্রুৱ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ধ্রুৱ std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ধ্রুৱ std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    স্থির StrGParams stringifyParams(ধ্রুৱ std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

সার্ৱজনিক:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, ধ্রুৱ AstModule* modp, ধ্রুৱ std::vector<AstVar*>& params,
                ধ্রুৱ std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = স্ৱভাৱ;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    ধ্রুৱ AstModule* modp() ধ্রুৱ { ঘূৰাই m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    স্থির bool stringParamPassable(ধ্রুৱ string& মান) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) ধ্রুৱ VL_MT_DISABLED;
    VStringList hierBlockArgs() ধ্রুৱ VL_MT_DISABLED;
    string hierPrefix() ধ্রুৱ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ধ্রুৱ ৱর্ণ* prefix,
                            ধ্রুৱ ৱর্ণ* suffix) ধ্রুৱ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ধ্রুৱ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ধ্রুৱ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ধ্রুৱ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ধ্রুৱ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ধ্রুৱ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    শূন্য writeCommandArgsFile(bool forMkJson) ধ্রুৱ VL_MT_DISABLED;
    শূন্য writeParametersFile() ধ্রুৱ VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) ধ্রুৱ VL_MT_DISABLED;
    string typeParametersFilename() ধ্রুৱ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ধ্রুৱ override { ঘূৰাই m_modp->prettyNameQ(); }
    std::string dotShape() ধ্রুৱ override { ঘূৰাই "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
সার্ৱজনিক:
    স্থির শূন্য createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
