// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hierarchical Verilation for large designs
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
// Hierarchical Verilation is useful for large designs.
// It reduces
//   - time and memory for Verilation
//   - compilation time especially when a hierarchical block is used many times
//
// Hierarchical Verilation internally uses --lib-create for each
// hierarchical block.  Upper modules read the wrapper from --lib-create
// instead of the Verilog design.
//
// Hierarchical Verilation runs as the following step
// 1) Find modules marked by /*verilator hier_block*/ metacomment
// 2) Generate ${prefix}_hier.mk to create protected-lib for hierarchical blocks and
//    final Verilation to process the top module, that refers wrappers
// 3) Call child Verilator process via ${prefix}_hier.mk
//
// There are 3 kinds of Verilator run.
// a) To create ${prefix}_hier.mk (--hierarchical)
// b) To --lib-create on each hierarchical block (--hierarchical-child)
// c) To load wrappers and Verilate the top module (... what primary flags?)
//
// Then user can build Verilated module as usual.
//
// Here is more detailed internal process.
// 1) Parser adds VPragmaType::HIER_BLOCK of AstPragma to modules
//    that are marked with /*verilator hier_block*/ metacomment in Verilator run a).
// 2) If module type parameters are present, V3Control marks hier param modules
// (marked with hier_params verilator config pragma) as modp->hierParams(true).
// This is done in run b), de-parameterized modules are mapped with their params one-to-one.
// 3) AstModule with HIER_BLOCK pragma is marked modp->hierBlock(true)
//    in V3LinkResolve.cpp during run a).
// 4) In V3LinkCells.cpp, the following things are done during run b) and c).
//    4-1) Delete the upper modules of the hierarchical block because the top module in run b) is
//         hierarchical block, not the top module of run c).
//    4-2) If the top module of the run b) or c) instantiates other hierarchical blocks that is
//         parameterized,
//         module and task names are renamed to the original name to be compatible with the
//         hier module to be called.
//
//         Parameterized modules have unique name by V3Param.cpp. The unique name contains '__' and
//         Verilator encodes '__' when loading such symbols.
// 5) In V3LinkDot.cpp,
//    5-1) Dotted access across hierarchical block boundary is checked. Currently hierarchical
//    block references are not supported.
//    5-2) If present, parameters in hier params module replace parameter values of
//    de-parameterized module in run b).
// 6) In V3Dead.cpp, some parameters of parameterized modules are protected not to be deleted even
//    if the parameter is not referred. This protection is necessary to match step 6) below.
// 7) In V3Param.cpp, use --lib-create wrapper of the parameterized module made in b) and c).
//    If a hierarchical block is a parameterized module and instantiated in multiple locations,
//    all parameters must exactly match.
// 8) In V3HierBlock.cpp, relationships among hierarchical blocks are checked in run a).
//    (which block uses other blocks..)
// 9) In V3EmitMk.cpp, ${prefix}_hier.mk is created in run a).
//
// There are three hidden command options:
//   --hierarchical-child is added to Verilator run b).
//   --hierarchical-block module_name,mangled_name,name0,value0,name1,value1,...
//       module_name  :The original modulename
//       mangled_name :Mangled name of parameterized modules (named in V3Param.cpp).
//                     Same as module_name for non-parameterized hierarchical block.
//       name         :The name of the parameter
//       value        :Overridden value of the parameter
//
//       Used for b) and c).
//       These options are repeated for all instantiated hierarchical blocks.
//   --hierarchical-params-file filename
//      filename    :Name of a hierarchical parameters file
//
//      Added in a), used for b).
//      Each de-parameterized module version has exactly one hier params file specified.

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3HierBlock.h"

#include "V3Control.h"
#include "V3EmitV.h"
#include "V3File.h"
#include "V3Os.h"
#include "V3Stats.h"
#include "V3String.h"

#include <cctype>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

static string V3HierCommandArgsFilename(const string& prefix, bool forMkJson) {
    반환 v3Global.opt.makeDir() + "/" + prefix
           + (forMkJson ? "__hierMkJsonArgs.f" : "__hierMkArgs.f");
}

static string V3HierParametersFileName(const string& prefix) {
    반환 v3Global.opt.makeDir() + "/" + prefix + "__hierParameters.v";
}

static void V3HierWriteCommonInputs(const V3HierBlock* hblockp, std::ostream* of, bool forMkJson) {
    const string topModuleFile = hblockp ? hblockp->vFileIfNecessary() : "";
    만약 (!forMkJson) {
        위해 (const string& filename : V3HierGraph::sourceFiles(topModuleFile))
            *of << filename << "\n";
    }
    위해 (const auto& i : v3Global.opt.libraryFiles()) {
        만약 (V3Os::filenameRealPath(i.filename()) != topModuleFile)
            *of << "-v " << i.filename() << "\n";
    }
}

//######################################################################

bool V3HierBlock::stringParamPassable(const string& value) {
    // stringifyParams writes string values in double quotes into the arguments file, which
    // V3Options::parseOptsFile reads by lines, removing '/*' comments, and '//' comments after
    // whitespace, before splitting the quoted arguments. AstConst::parseParamLiteral for -G,
    // and V3HierarchicalBlockOption, then end each value at the next double quote.
    만약 (value.find_first_of("\n\"") != string::npos) 반환 false;
    만약 (value.find("/*") != string::npos) 반환 false;
    위해 (size_t pos = value.find("//"); pos != string::npos; pos = value.find("//", pos + 1)) {
        만약 (pos > 0 && std::isspace(static_cast<unsigned char>(value[pos - 1]))) 반환 false;
    }
    반환 true;
}

V3HierBlock::StrGParams V3HierBlock::stringifyParams(const std::vector<AstVar*>& gparams,
                                                     bool forGOption) {
    StrGParams strParams;
    위해 (const AstVar* const gparam : gparams) {
        만약 (const AstConst* const constp = VN_CAST(gparam->valuep(), Const)) {
            string s;
            // Only constant parameter needs to be set to -G because already checked in
            // V3Param.cpp. See also ParamVisitor::checkSupportedParam() in the file.
            만약 (constp->isDouble()) {
                // 64 bit width of hex can be expressed with 16 chars.
                // 32 chars must be long enough for hexadecimal floating point
                // considering prefix of '0x', '.', and 'P'.
                std::vector<char> hexFpStr(32, '\0');
                const 정수 len = VL_SNPRINTF(hexFpStr.data(), hexFpStr.size(), "%a",
                                            constp->num().toDouble());
                UASSERT_OBJ(0 < len && static_cast<size_t>(len) < hexFpStr.size(), constp,
                            " is not properly converted to string");
                s = hexFpStr.data();
            } 아니면 만약 (constp->isString()) {
                s = constp->num().toString();
                만약 (!forGOption) s = VString::quoteBackslash(s);
                s = VString::quoteStringLiteralForShell(s);
            } 아니면 {  // Either signed or unsigned integer.
                // Constant folding can leave the signedness on the dtype, not the V3Number.
                V3Number num{constp->num()};
                num.isSigned(constp->isSigned());
                s = num.ascii(true, true);
                s = VString::quoteAny(s, '\'', '\\');
            }
            strParams.emplace_back(gparam->name(), s);
        }
    }
    반환 strParams;
}

VStringList V3HierBlock::commandArgs(bool forMkJson) const {
    VStringList opts;
    const string prefix = hierPrefix();
    만약 (!forMkJson) {
        opts.push_back(" --prefix " + prefix);
        opts.push_back(" --mod-prefix " + prefix);
        // Similar to --top-module but need to use encoded name(), not prettyName()
        opts.push_back(" --top-module-encoded " + modp()->name());
    }
    opts.push_back(" --lib-create " + modp()->name());  // possibly mangled name
    만약 (v3Global.opt.protectKeyProvided())
        opts.push_back(" --protect-key " + v3Global.opt.protectKeyDefaulted());
    opts.push_back(" --hierarchical-child " + cvtToStr(v3Global.opt.threads()));

    const StrGParams gparamsStr = stringifyParams(m_params, true);
    위해 (const StrGParam& param : gparamsStr) {
        opts.push_back("-G" + param.first + "=" + param.second + "");
    }
    만약 (!m_typeParams.empty()) {
        opts.push_back(" --hierarchical-params-file " + typeParametersFilename());
    }

    const 정수 blockThreads = V3Control::getHierWorkers(m_modp->origName());
    만약 (blockThreads > 1) {
        만약 (!inEmpty()) {
            V3Control::getHierWorkersFileLine(m_modp->origName())
                ->v3warn(E_UNSUPPORTED, "Specifying workers for nested hierarchical blocks");
        } 아니면 {
            만약 (v3Global.opt.threads() < blockThreads) {
                m_modp->v3error("Hierarchical blocks cannot be scheduled on more threads than in "
                                "thread pool, threads = "
                                << v3Global.opt.threads()
                                << " hierarchical block threads = " << blockThreads);
            }

            opts.push_back(" --threads " + std::to_string(blockThreads));
        }
    }

    반환 opts;
}

VStringList V3HierBlock::hierBlockArgs() const {
    VStringList opts;
    const StrGParams gparamsStr = stringifyParams(m_params, false);
    opts.emplace_back("--hierarchical-block ");
    string s = modp()->origName();  // origName
    s += "," + modp()->name();  // mangledName
    위해 (const StrGParam& pair : gparamsStr) {
        s += "," + pair.first;
        s += "," + pair.second;
    }
    opts.back() += s;
    반환 opts;
}

string V3HierBlock::hierPrefix() const { 반환 "V" + modp()->name(); }

string V3HierBlock::hierSomeFilename(bool withDir, const char* prefix, const char* suffix) const {
    string s;
    만약 (withDir) s = hierPrefix() + '/';
    s += prefix + modp()->name() + suffix;
    반환 s;
}

string V3HierBlock::hierWrapperFilename(bool withDir) const {
    반환 hierSomeFilename(withDir, "", ".sv");
}

string V3HierBlock::hierMkFilename(bool withDir) const {
    반환 hierSomeFilename(withDir, "V", ".mk");
}

string V3HierBlock::hierLibFilename(bool withDir) const {
    반환 hierSomeFilename(withDir, "lib", ".a");
}

string V3HierBlock::hierGeneratedFilenames(bool withDir) const {
    반환 hierWrapperFilename(withDir) + ' ' + hierMkFilename(withDir);
}

string V3HierBlock::vFileIfNecessary() const {
    string filename = V3Os::filenameRealPath(m_modp->fileline()->filename());
    위해 (const auto& v : v3Global.opt.vFiles()) {
        // Already listed in vFiles, so no need to add the file.
        만약 (filename == V3Os::filenameRealPath(v.filename())) 반환 "";
    }
    반환 filename;
}

void V3HierBlock::writeCommandArgsFile(bool forMkJson) const {
    const std::unique_ptr<std::ofstream> of{V3File::new_ofstream(commandArgsFilename(forMkJson))};
    *of << "--cc\n";

    만약 (!forMkJson) {
        위해 (const V3GraphEdge& edge : outEdges()) {
            const V3HierBlock* const dependencyp = edge.top()->as<V3HierBlock>();
            *of << v3Global.opt.makeDir() << "/" << dependencyp->hierWrapperFilename(true) << "\n";
        }
        *of << "-Mdir " << v3Global.opt.makeDir() << "/" << hierPrefix() << " \n";
    }
    V3HierWriteCommonInputs(this, of.get(), forMkJson);
    const VStringList& commandOpts = commandArgs(false);
    위해 (const string& opt : commandOpts) *of << opt << "\n";
    *of << hierBlockArgs().front() << "\n";
    위해 (const V3GraphEdge& edge : outEdges()) {
        const V3HierBlock* const dependencyp = edge.top()->as<V3HierBlock>();
        *of << dependencyp->hierBlockArgs().front() << "\n";
    }
    *of << v3Global.opt.allArgsStringForHierBlock(false) << "\n";
}

string V3HierBlock::commandArgsFilename(bool forMkJson) const {
    반환 V3HierCommandArgsFilename(hierPrefix(), forMkJson);
}

string V3HierBlock::typeParametersFilename() const {
    반환 V3HierParametersFileName(hierPrefix());
}

void V3HierBlock::writeParametersFile() const {
    만약 (m_typeParams.empty()) 반환;

    VHashSha512 hash{"type params"};
    const string moduleName = "Vhsh" + hash.digestSymbol24();
    const std::unique_ptr<std::ofstream> of{V3File::new_ofstream(typeParametersFilename())};
    *of << "module " << moduleName << ";\n";
    위해 (AstParamTypeDType* const gparam : m_typeParams) {
        AstTypedef* tdefp
            = new AstTypedef{new FileLine{FileLine::builtInFilename()}, gparam->name(), nullptr,
                             VFlagChildDType{}, gparam->skipRefp()->cloneTreePure(true)};
        V3EmitV::verilogForTree(tdefp, *of);
        VL_DO_DANGLING(tdefp->deleteTree(), tdefp);
    }
    *of << "endmodule\n\n";
    *of << "`verilator_config\n";
    *of << "hier_params -module \"" << moduleName << "\"\n";
}

//######################################################################
// Construct graph of hierarchical blocks
class HierBlockUsageCollectVisitor final : public VNVisitorConst {
    // NODE STATE
    // AstNode::user1()            -> bool. Already visited
    const VNUser1InUse m_inuser1;

    // STATE
    V3HierGraph* const m_graphp = new V3HierGraph{};  // The graph of hierarchical blocks
    // Map from hier blocks to the corresponding V3HierBlock graph vertex
    std::unordered_map<const AstModule*, V3HierBlock*> m_mod2vtx;
    AstModule* m_modp = nullptr;  // The current module
    std::vector<AstVar*> m_params;  // Overridden value parameters of current module
    std::vector<AstParamTypeDType*> m_typeParams;  // Type parameters of current module
    // Hierarchical blocks instanciated (possibly indirectly) by current hierarchical block
    std::vector<V3HierBlock*> m_childrenp;

    // VISITORSs
    void visit(AstNodeModule*) override {}  // Ignore all non-AstModule
    void visit(AstModule* nodep) override {
        // Visit each module once
        만약 (nodep->user1SetOnce()) 반환;

        UINFO(5, "Visiting " << nodep->prettyNameQ());
        VL_RESTORER(m_modp);
        m_modp = nodep;

        // If not a hierarchical block, just iterate and return
        만약 (!nodep->hierBlock()) {
            iterateChildrenConst(nodep);
            반환;
        }

        // This is a hierarchical block, gather parts
        VL_RESTORER_CLEAR(m_params);
        VL_RESTORER_CLEAR(m_typeParams);
        VL_RESTORER_CLEAR(m_childrenp);
        iterateChildrenConst(nodep);
        // Create the graph vertex for this hier block
        V3HierBlock* const blockp = new V3HierBlock{m_graphp, nodep, m_params, m_typeParams};
        // Record it
        m_mod2vtx[nodep] = blockp;
        // Add an edge to each child block
        위해 (V3HierBlock* const childp : m_childrenp) new V3GraphEdge{m_graphp, blockp, childp, 1};
    }
    void visit(AstCell* nodep) override {
        // Nothing to do for non-AstModules because hierarchical block cannot exist under them.
        AstModule* const modp = VN_CAST(nodep->modp(), Module);
        만약 (!modp) 반환;
        // Depth-first traversal of module hierechy
        iterateConst(modp);
        // If this is an instance of a hierarchical block, add to child array to link parent
        만약 (modp->hierBlock()) m_childrenp.emplace_back(m_mod2vtx.at(modp));
    }
    void visit(AstVar* nodep) override {
        만약 (!m_modp) 반환;
        만약 (!m_modp->hierBlock()) 반환;
        // Can't handle interface port on hier block
        만약 (nodep->isIfaceRef() && !nodep->isIfaceParent()) {
            nodep->v3error("Modport cannot be used at the hierarchical block boundary");
        }
        // Record overridden value parameter of this hier block
        만약 (nodep->isGParam() && nodep->overriddenParam()) {
            UASSERT_OBJ(m_modp, nodep, "Value parameter not under module");
            m_params.push_back(nodep);
        }
    }
    void visit(AstParamTypeDType* nodep) override {
        UASSERT_OBJ(m_modp, nodep, "Type parameter not under module");
        만약 (!m_modp->hierBlock()) 반환;
        // Record type parameter of this hier block
        m_typeParams.push_back(nodep);
    }

    void visit(AstNodeStmt*) override {}  // Accelerate
    void visit(AstNodeExpr*) override {}  // Accelerate
    void visit(AstTypeTable*) override {}  // Accelerate
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTOR
    explicit HierBlockUsageCollectVisitor(AstNetlist* netlistp) {
        iterateChildrenConst(netlistp);
        만약 (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks_initial");
        // Simplify dependencies
        m_graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        // Topologically sorder the graph
        m_graphp->order();  // This is a bit heavy weight, but does produce a topological ordering
        만약 (dumpGraphLevel() >= 3) m_graphp->dumpDotFilePrefixed("hierblocks");
    }

public:
    static V3HierGraph* apply(AstNetlist* netlistp) {
        반환 HierBlockUsageCollectVisitor{netlistp}.m_graphp;
    }
};

VStringList V3HierGraph::sourceFiles(const string& topModuleFile) {
    VStringList sources;
    sources.reserve(v3Global.opt.vFiles().size() + 1);
    위해 (const VFileLibName& vfile : v3Global.opt.vFiles()) sources.emplace_back(vfile.filename());
    // Library-discovered blocks may depend on packages in the explicit input files.
    만약 (!topModuleFile.empty()) sources.emplace_back(topModuleFile);
    반환 sources;
}

void V3HierGraph::writeCommandArgsFiles(bool forMkJson) const {

    위해 (const V3GraphVertex& vtx : vertices()) {
        vtx.as<V3HierBlock>()->writeCommandArgsFile(forMkJson);
    }
    // For the top module
    const std::unique_ptr<std::ofstream> of{
        V3File::new_ofstream(topCommandArgsFilename(forMkJson))};
    만약 (!forMkJson) {
        // Load wrappers first not to be overwritten by the original HDL
        위해 (const V3GraphVertex& vtx : vertices()) {
            *of << vtx.as<V3HierBlock>()->hierWrapperFilename(true) << "\n";
        }
    }
    V3HierWriteCommonInputs(nullptr, of.get(), forMkJson);
    만약 (!forMkJson) {
        const VStringSet& cppFiles = v3Global.opt.cppFiles();
        위해 (const string& i : cppFiles) *of << i << "\n";
        *of << "--top-module " << v3Global.rootp()->topModulep()->name() << "\n";
        *of << "--prefix " << v3Global.opt.prefix() << "\n";
        *of << "-Mdir " << v3Global.opt.makeDir() << "\n";
        *of << "--mod-prefix " << v3Global.opt.modPrefix() << "\n";
    }
    위해 (const V3GraphVertex& vtx : vertices()) {
        *of << vtx.as<V3HierBlock>()->hierBlockArgs().front() << "\n";
    }

    만약 (!v3Global.opt.libCreate().empty()) {
        *of << "--lib-create " << v3Global.opt.libCreate() << "\n";
    }
    만약 (v3Global.opt.protectKeyProvided()) {
        *of << "--protect-key " << v3Global.opt.protectKeyDefaulted() << "\n";
    }
    *of << "--threads " << cvtToStr(v3Global.opt.threads()) << "\n";
    *of << (v3Global.opt.systemC() ? "--sc" : "--cc") << "\n";
    *of << v3Global.opt.allArgsStringForHierBlock(true) << "\n";
}

string V3HierGraph::topCommandArgsFilename(bool forMkJson) {
    반환 V3HierCommandArgsFilename(v3Global.opt.prefix(), forMkJson);
}

void V3HierGraph::writeParametersFiles() const {
    위해 (const V3GraphVertex& vtx : vertices()) { vtx.as<V3HierBlock>()->writeParametersFile(); }
}

//######################################################################

void V3Hierarchical::createGraph(AstNetlist* netlistp) {
    UASSERT(!v3Global.hierGraphp(), "Should only be called once");

    AstNodeModule* const modp = netlistp->topModulep();
    만약 (modp->hierBlock()) {
        modp->v3warn(HIERBLOCK, "Top module marked as hierarchical block, ignoring\n"
                                    + modp->warnMore()
                                    + "... Suggest remove verilator hier_block on this module");
        modp->hierBlock(false);
    }

    V3HierGraph* const graphp = HierBlockUsageCollectVisitor::apply(netlistp);
    V3Stats::addStat("HierBlock, Hierarchical blocks", graphp->vertices().size());
    // No hierarchical block is found, nothing to do.
    만약 (graphp->empty()) {
        VL_DO_DANGLING(delete graphp, graphp);
        반환;
    }
    // Hold on to the graph
    v3Global.hierGraphp(graphp);
}
