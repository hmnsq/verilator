// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "V3PchAstMT.h"

#include "V3Control.h"

#include "V3InstrCount.h"
#include "V3Os.h"
#include "V3String.h"

#include <memory>
#include <set>
#include <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
class V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<const std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<const std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

public:
    V3ControlWildcardResolver() = default;
    ~V3ControlWildcardResolver() = default;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    void update(const V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        const V3LockGuard lock{m_mutex};
        const V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        សម្រាប់ (const auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(const string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        const V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ត្រឡប់ m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(const string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        const V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        const auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ប្រសិនបើ (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            const string slashName = V3Os::filenameSlashPath(name);
            សម្រាប់ (const auto& patEnt : m_mapPatterns) {
                ប្រសិនបើ (VString::wildmatch(slashName, patEnt.first)) {
                    ប្រសិនបើ (!entryr) entryr.reset(new T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            ប្រសិនបើ (entryr) entryr->updateFinalize();
        }
        ត្រឡប់ entryr.get();
    }
};

// List of attributes for variables
class V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
public:
    // Add new attribugte
    void add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    void update(const V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.insert(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    void updateFinalize() {}
    // Apply all attributes to the variable
    void apply(AstVar* varp) const {
        សម្រាប់ (const VAttrType attr : m_attrs) {
            varp->addAttrsp(new AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

class WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<const std::string, bool> m_mapPatterns;  // Pattern match results
    std::deque<string> m_lines;  // Source text lines

    // METHODS
    static WildcardContents& s() {  // Singleton
        static WildcardContents s_s;
        ត្រឡប់ s_s;
    }
    void clearCacheImp() { m_mapPatterns.clear(); }
    void pushTextImp(const string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        const string leftover = m_lines.back() + text;
        m_lines.pop_back();

        // Insert line-by-line
        string::size_type line_start = 0;
        ខណៈពេល (true) {
            const string::size_type line_end = leftover.find('\n', line_start);
            ប្រសិនបើ (line_end != string::npos) {
                const string oneline(leftover, line_start, line_end - line_start + 1);
                ប្រសិនបើ (oneline.size() > 1) m_lines.push_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_lines.size() - 1) << "]: " << oneline);
                line_start = line_end + 1;
            } បើមិនដូច្នេះ {
                bha_nga;
            }
        }
        // Keep leftover for next time
        m_lines.emplace_back(string(leftover, line_start));  // Might be ""
        clearCacheImp();
    }

    bool resolveUncachedImp(const string& contentsRegexp) {
        សម្រាប់ (const string& i : m_lines) {
            ប្រសិនបើ (VString::wildmatch(i, contentsRegexp)) ត្រឡប់ true;
        }
        ត្រឡប់ false;
    }
    bool resolveCachedImp(const string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ប្រសិនបើ (contentsRegexp.empty()) ត្រឡប់ true;
        const auto pair = m_mapPatterns.emplace(contentsRegexp, false);
        bool& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ប្រសិនបើ (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        ត្រឡប់ entryr;
    }

public:
    WildcardContents() {
        m_lines.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = default;
    // Return true iff contentsRegexp in parsed contents
    static bool resolve(const string& contentsRegexp) {
        ត្រឡប់ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    static void pushText(const string& text) { s().pushTextImp(text); }
};

//######################################################################
// Function or task: Have variables and properties

class V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    bool m_noinline = false;  // Don't inline function/task
    bool m_public = false;  // Public function/task

public:
    V3ControlFTask() = default;
    void update(const V3ControlFTask& f) {
        // Don't overwrite true with false
        ប្រសិនបើ (f.m_noinline) m_noinline = true;
        ប្រសិនបើ (f.m_public) m_public = true;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    void updateFinalize() {}

    V3ControlVarResolver& params() { ត្រឡប់ m_params; }
    V3ControlVarResolver& ports() { ត្រឡប់ m_ports; }
    V3ControlVarResolver& vars() { ត្រឡប់ m_vars; }

    void setNoInline(bool set) { m_noinline = set; }
    void setPublic(bool set) { m_public = set; }

    void apply(AstNodeFTask* ftaskp) const {
        ប្រសិនបើ (m_noinline)
            ftaskp->addStmtsp(new AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ប្រសិនបើ (m_public)
            ftaskp->addStmtsp(new AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

class V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    bool m_hasFsmRegisterWrapper = false;  // Whether descriptor has been set
    bool m_inline = false;  // Whether to force the inline
    bool m_inlineValue = false;  // The inline value (on/off)

public:
    V3ControlModule() = default;

    void update(const V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ប្រសិនបើ (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = true;
        }
        សម្រាប់ (const string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.insert(i);
        ប្រសិនបើ (!m_inline) {
            m_inline = m.m_inline;
            m_inlineValue = m.m_inlineValue;
        }
        សម្រាប់ (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.insert(*it);
        }
    }
    void updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ត្រឡប់ m_tasks; }
    V3ControlVarResolver& params() { ត្រឡប់ m_params; }
    V3ControlVarResolver& ports() { ត្រឡប់ m_ports; }
    V3ControlVarResolver& vars() { ត្រឡប់ m_vars; }

    void addCoverageBlockOff(const string& name) { m_coverageOffBlocks.insert(name); }
    void setInline(bool set) {
        m_inline = true;
        m_inlineValue = set;
    }
    void setFsmRegisterWrapper(FileLine* fl, const V3Control::FsmRegisterWrapper& desc) {
        ប្រសិនបើ (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << AstNode::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = true;
    }
    const V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() const {
        ត្រឡប់ m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    void addModulePragma(VPragmaType pragma) { m_modPragmas.insert(pragma); }

    void apply(AstNodeModule* modp) {
        ប្រសិនបើ (m_inline) {
            const VPragmaType type
                = m_inlineValue ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            AstNode* const nodep = new AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        សម្រាប់ (const auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ប្រសិនបើ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(true);
            AstNode* const nodep = new AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    void applyBlock(AstGenBlock* nodep) {
        const VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ប្រសិនបើ (!nodep->unnamed()) {
            សម្រាប់ (const string& i : m_coverageOffBlocks) {
                ប្រសិនបើ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(new AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    void applyBlock(AstNodeBlock* nodep) {
        const VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        FileLine* const flp = nodep->fileline();
        ប្រសិនបើ (!nodep->unnamed()) {
            សម្រាប់ (const string& i : m_coverageOffBlocks) {
                ប្រសិនបើ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(new AstStmtPragma{flp, new AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
class V3ControlIgnoresLine final {
public:
    const ចំនួនគត់ m_lineMin;  // Minimum line number to make change at
    const ចំនួនគត់ m_lineMax;  // Maximum line number to make change at (inclusive)
    const V3ErrorCode m_code;  // Error code
    const bool m_on;  // True to enable message
    V3ControlIgnoresLine(V3ErrorCode code, ចំនួនគត់ linemin, ចំនួនគត់ linemax, bool on)
        : m_lineMin{linemin}
        , m_lineMax{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3ControlIgnoresLine() = default;
    bool everyLine() const { ត្រឡប់ m_lineMin == 0 && m_lineMax == 0; }
    bool inRange(ចំនួនគត់ lineno) const {
        ត្រឡប់ (lineno >= m_lineMin && lineno <= m_lineMax) || everyLine();
    }
};
std::ostream& operator<<(std::ostream& os, const V3ControlIgnoresLine& rhs) {
    ត្រឡប់ os << rhs.m_lineMin << "-" << rhs.m_lineMax << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
static std::vector<V3ControlIgnoresLine> controlIgnLines;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

class VIntervalTree final {
    sa_nracanaa Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        ចំនួនគត់ m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(ចំនួនគត់ center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_everyLines;  // All-line disables

private:
    std::unique_ptr<Entry> buildTree(const IgnIndices& points) {
        ប្រសិនបើ (points.empty()) ត្រឡប់ nullptr;

        ចំនួនគត់ minval = std::numeric_limits<ចំនួនគត់>::max();
        ចំនួនគត់ maxval = std::numeric_limits<ចំនួនគត់>::min();
        សម្រាប់ (const auto& it : points) {
            const V3ControlIgnoresLine& cign = controlIgnLines[it];
            ប្រសិនបើ (cign.everyLine()) {
                m_everyLines.emplace_back(it);
                continue;
            }
            minval = std::min(minval, cign.m_lineMin);
            maxval = std::max(maxval, cign.m_lineMax);
        }

        const ចំនួនគត់ center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(new Entry{center});

        IgnIndices leftPoints;
        IgnIndices rightPoints;
        សម្រាប់ (const auto& it : points) {
            const V3ControlIgnoresLine& cign = controlIgnLines[it];
            ប្រសិនបើ (cign.everyLine()) continue;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ប្រសិនបើ (cign.m_lineMax < center)
                leftPoints.emplace_back(it);
            បើមិនដូច្នេះ ប្រសិនបើ (cign.m_lineMin > center)
                rightPoints.emplace_back(it);
            បើមិនដូច្នេះ {
                entp->m_byMin.push_back(it);
                entp->m_byMax.push_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            ត្រឡប់ controlIgnLines[a].m_lineMin < controlIgnLines[b].m_lineMin;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            ត្រឡប់ controlIgnLines[a].m_lineMax > controlIgnLines[b].m_lineMax;
        });

        entp->m_leftp = buildTree(leftPoints);
        entp->m_rightp = buildTree(rightPoints);
        ត្រឡប់ entp;
    }

    void findTree(ចំនួនគត់ lineno, Entry* entp, IgnIndices& resultsr, ចំនួនគត់& nextChanger) const {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ប្រសិនបើ (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            សម្រាប់ (const auto& it : entp->m_byMin) {
                const V3ControlIgnoresLine& cign = controlIgnLines[it];
                ប្រសិនបើ (cign.m_lineMin <= lineno) {
                    resultsr.emplace_back(it);
                } បើមិនដូច្នេះ {
                    nextChanger = std::min(nextChanger, cign.m_lineMin);
                    bha_nga;
                }
            }
            ប្រសិនបើ (entp->m_leftp) findTree(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } បើមិនដូច្នេះ ប្រសិនបើ (lineno > entp->m_center) {
            សម្រាប់ (const auto& it : entp->m_byMax) {
                const V3ControlIgnoresLine& cign = controlIgnLines[it];
                ប្រសិនបើ (cign.m_lineMax >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_lineMax + 1);
                    resultsr.emplace_back(it);
                } បើមិនដូច្នេះ {
                    bha_nga;
                }
            }
            ប្រសិនបើ (entp->m_rightp) findTree(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } បើមិនដូច្នេះ {
            សម្រាប់ (const auto& it : entp->m_byMin) {
                const V3ControlIgnoresLine& cign = controlIgnLines[it];
                resultsr.emplace_back(it);
                ប្រសិនបើ (cign.m_lineMax >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_lineMax + 1);
            }
            ប្រសិនបើ (entp->m_leftp) findTree(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            ប្រសិនបើ (entp->m_rightp) findTree(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

public:
    // CONSTRUCTORS
    VIntervalTree() = default;
    ~VIntervalTree() = default;
    // METHODS
    void clear() { m_rootp = nullptr; }
    void build(const IgnIndices& points) {
        clear();
        m_rootp = buildTree(points);
    }
    void find(ចំនួនគត់ lineno, IgnIndices& resultsr, ចំនួនគត់& nextChanger) const {
        resultsr = m_everyLines;
        nextChanger = std::numeric_limits<ចំនួនគត់>::max();

        ប្រសិនបើ (m_rootp) findTree(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    static void selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<ចំនួនគត់, ចំនួនគត់>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        សម្រាប់ (const auto& it : points) {
            controlIgnLines.emplace_back(
                V3ControlIgnoresLine{V3ErrorCode::I_LINT, it.first, it.second, true});
            data.emplace_back(static_cast<uint32_t>(controlIgnLines.size() - 1));
        }
        VIntervalTree tree;
        tree.build(data);

        IgnIndices results;
        ចំនួនគត់ nextChange = 0;
        tree.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        tree.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        tree.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        tree.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        tree.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        tree.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        tree.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        tree.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<ចំនួនគត់>::max());
        //
        points = {{0, 0}};
        សម្រាប់ (const auto& it : points) {
            controlIgnLines.emplace_back(
                V3ControlIgnoresLine{V3ErrorCode::I_LINT, it.first, it.second, true});
            data.emplace_back(static_cast<uint32_t>(controlIgnLines.size() - 1));
        }
        tree.build(data);
        //
        tree.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3ControlLineAttribute = std::bitset<VPragmaType::_ENUM_SIZE>;

class WaiverSetting final {
public:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, const string& contents, const string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = default;
    WaiverSetting& operator=(const WaiverSetting& rhs) = default;
};

// File entity
class V3ControlFile final {
    using LineAttrMap = std::map<ចំនួនគត់, V3ControlLineAttribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    LineAttrMap m_lineAttrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VIntervalTree m_intervalTree;  // Tree of indices to IgnLines

    sa_nracanaa {
        ចំនួនគត់ filenameno = -1;  // Last filename
        ចំនួនគត់ lineno = -1;  // Last linenumber
        ចំនួនគត់ nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    bool lineMatch(ចំនួនគត់ lineno, VPragmaType type) {
        ប្រសិនបើ (m_lineAttrs.find(0) != m_lineAttrs.end() && m_lineAttrs[0][type]) ត្រឡប់ true;
        ប្រសិនបើ (m_lineAttrs.find(lineno) == m_lineAttrs.end()) ត្រឡប់ false;
        ត្រឡប់ m_lineAttrs[lineno][type];
    }

public:
    V3ControlFile() = default;
    void update(const V3ControlFile& file) {
        // Copy in all attributes and waivers
        សម្រាប់ (const auto& itr : file.m_lineAttrs) m_lineAttrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.insert(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.insert(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    void updateFinalize() { m_intervalTree.build(m_ignIndices); }
    void addLineAttribute(ចំនួនគត់ lineno, VPragmaType attr) { m_lineAttrs[lineno].set(attr); }
    void addIgnore(V3ErrorCode code, ចំនួនគត់ min, ចំនួនគត់ max, bool on) {
        controlIgnLines.emplace_back(V3ControlIgnoresLine{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(controlIgnLines.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    void addIgnoreMatch(V3ErrorCode code, const string& contents, const string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        ប្រសិនបើ (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    void applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        const VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ប្រសិនបើ (lineMatch(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(new AstPragma{nodep->fileline(), pragma});
        }
    }
    void applyBlock(AstNodeBlock* nodep) {
        // Apply to block at this line
        const VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        FileLine* const flp = nodep->fileline();
        ប្រសិនបើ (lineMatch(flp->lineno(), pragma)) {
            nodep->addStmtsp(new AstStmtPragma{flp, new AstPragma{flp, pragma}});
        }
    }
    void applyCase(AstCase* nodep) {
        // Apply to this case at this line
        const ចំនួនគត់ lineno = nodep->fileline()->lineno();
        ប្រសិនបើ (lineMatch(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(true);
        ប្រសិនបើ (lineMatch(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(true);
    }
    void applyIgnores(FileLine* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ប្រសិនបើ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            ត្រឡប់;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ប្រសិនបើ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ត្រឡប់;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<ចំនួនគត់>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_intervalTree.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        សម្រាប់ (const auto& it : results) {
            const V3ControlIgnoresLine& cign = controlIgnLines[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    bool waive(V3ErrorCode code, const string& message) {
        ប្រសិនបើ (code.hardError()) ត្រឡប់ false;
        សម្រាប់ (const auto& itr : m_waivers) {
            ប្រសិនបើ ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                ត្រឡប់ true;
            }
        }
        ត្រឡប់ false;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

class V3ControlScopeTraceEntry final {
public:
    const string m_scope;  // Scope or regexp to match
    const bool m_on;  // True to enable message
    ចំនួនគត់ m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(const string& scope, bool on, ចំនួនគត់ levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = default;
    bool operator<(const V3ControlScopeTraceEntry& other) const {
        ប្រសិនបើ (m_on < other.m_on) ត្រឡប់ true;
        ប្រសិនបើ (m_on > other.m_on) ត្រឡប់ false;
        ប្រសិនបើ (m_levels < other.m_levels) ត្រឡប់ true;
        ប្រសិនបើ (m_levels > other.m_levels) ត្រឡប់ false;
        ត្រឡប់ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
class V3ControlScopeTraceEntryMatch final {
public:
    const V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    const string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(const V3ControlScopeTraceEntry* entryp, const string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    bool operator<(const V3ControlScopeTraceEntryMatch& other) const {
        ប្រសិនបើ (m_entryp < other.m_entryp) ត្រឡប់ true;
        ប្រសិនបើ (m_entryp > other.m_entryp) ត្រឡប់ false;
        ត្រឡប់ m_scopepart < other.m_scopepart;
    }
};

class V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, bool> m_matchCache;  // Matching entries for speed

public:
    void addScopeTraceOn(bool on, const string& scope, ចំនួនគត់ levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    bool getEntryMatch(const V3ControlScopeTraceEntry* entp, const string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        const V3ControlScopeTraceEntryMatch key{entp, scopepart};
        const auto pair = m_matchCache.emplace(key, false);
        ប្រសិនបើ (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        ត្រឡប់ pair.first->second;
    }

    bool getScopeTraceOn(const string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        ចំនួនគត់ maxLevel = 1;
        សម្រាប់ (const auto& ch : scope) {
            ប្រសិនបើ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        bool enabled = true;
        សម្រាប់ (const auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            ចំនួនគត់ partLevel = 1;
            សម្រាប់ (string::size_type partEnd = 0; true;) {
                partEnd = scope.find('.', partEnd + 1);
                ប្រសិនបើ (partEnd == string::npos) partEnd = scope.length();
                const std::string scopepart = scope.substr(0, partEnd);
                ប្រសិនបើ (getEntryMatch(&ent, scopepart)) {
                    const bool levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    ប្រសិនបើ (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    bha_nga;
                }
                ប្រសិនបើ (partEnd == scope.length()) bha_nga;
                ++partLevel;
            }
        }
        ត្រឡប់ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

class V3ControlResolverHierWorkerEntry final {
    const ចំនួនគត់ m_workers;  // Number of workers requested for the hierarchical block
    FileLine* const m_flp;  // Location the worker count was specified at

public:
    explicit V3ControlResolverHierWorkerEntry(ចំនួនគត់ workers, FileLine* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    ចំនួនគត់ workers() const { ត្រឡប់ m_workers; }
    FileLine* flp() const { ត្រឡប់ m_flp; }
};

class V3ControlResolver final {
    ga_nanaa ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    FileLine* m_profileFileLine = nullptr;  // Location profile_data was read from

    V3ControlResolver() = default;
    ~V3ControlResolver() = default;

public:
    static V3ControlResolver& s() {
        static V3ControlResolver s_singleton;
        ត្រឡប់ s_singleton;
    }
    V3ControlModuleResolver& modules() { ត្រឡប់ m_modules; }
    V3ControlFileResolver& files() { ត្រឡប់ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ត្រឡប់ m_scopeTraces; }

    void addProfileData(FileLine* fl, const string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    void addProfileData(FileLine* fl, const string& model, const string& key, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        ប្រសិនបើ (!m_profileFileLine) m_profileFileLine = fl;
        ប្រសិនបើ (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        ប្រសិនបើ (mode == MTASK) {
            m_profileData[model][key] += cost;
        } បើមិនដូច្នេះ ប្រសិនបើ (mode == HIER_DPI) {
            m_profileData[model][key] = std::max(m_profileData[model][key], cost);
        }
        m_mode |= mode;
    }
    bool containsMTaskProfileData() const { ត្រឡប់ m_mode & MTASK; }
    uint64_t getProfileData(const string& hierDpi) const {
        // Empty key for hierarchical DPI wrapper costs.
        ត្រឡប់ getProfileData(hierDpi, "");
    }
    void addHierWorkers(FileLine* flp, const string& model, ចំនួនគត់ workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    ចំនួនគត់ getHierWorkers(const string& model) const {
        const auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        ត្រឡប់ mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    FileLine* getHierWorkersFileLine(const string& model) const {
        const auto mit = m_hierWorkers.find(model);
        ត្រឡប់ mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(const string& model, const string& key) const {
        const auto mit = m_profileData.find(model);
        ប្រសិនបើ (mit == m_profileData.cend()) ត្រឡប់ 0;
        const auto it = mit->second.find(key);
        ប្រសិនបើ (it == mit->second.cend()) ត្រឡប់ 0;
        ត្រឡប់ it->second;
    }
    FileLine* getProfileDataFileLine() const { ត្រឡប់ m_profileFileLine; }  // Maybe null
    static uint64_t getCurrentHierBlockCost() {
        ប្រសិនបើ (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            ត្រឡប់ cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* const netlistp = v3Global.rootp();
        uint64_t cost = 0;
        សម្រាប់ (ចំនួនគត់ i = 0; i < VEval::_ENUM_END; ++i) {
            const VEval eval{i};
            ប្រសិនបើ (eval.slow()) continue;
            cost += V3InstrCount::count(netlistp->evalFuncp(eval), false);
        }
        UINFO(9, "Evaluating cost: " << cost);
        ត្រឡប់ cost;
    }
};

//######################################################################
// V3Control

void V3Control::addCaseFull(const string& filename, ចំនួនគត់ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.addLineAttribute(lineno, VPragmaType::FULL_CASE);
}
void V3Control::addCaseParallel(const string& filename, ចំនួនគត់ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.addLineAttribute(lineno, VPragmaType::PARALLEL_CASE);
}
void V3Control::addCoverageBlockOff(const string& filename, ចំនួនគត់ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.addLineAttribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
void V3Control::addCoverageBlockOff(const string& module, const string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

void V3Control::addHierWorkers(FileLine* fl, const string& model, ចំនួនគត់ workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

void V3Control::addFsmRegisterWrapper(FileLine* fl, const string& module, const string& d,
                                      const string& q, const string& clock, const string& reset,
                                      const string& resetValue) {
    string missing;
    ប្រសិនបើ (module.empty()) missing += "-module";
    ប្រសិនបើ (d.empty()) missing = VString::dot(missing, ", ", "-d");
    ប្រសិនបើ (q.empty()) missing = VString::dot(missing, ", ", "-q");
    ប្រសិនបើ (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    ប្រសិនបើ (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ត្រឡប់;
    }

    const FsmRegisterWrapper desc{module, d, q, clock, reset, resetValue};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

void V3Control::addIgnore(V3ErrorCode code, bool on, const string& filename, ចំនួនគត់ min, ចំនួនគត់ max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    ប្រសិនបើ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        FileLine::globalWarnOff(code, !on);
    } បើមិនដូច្នេះ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
void V3Control::addIgnoreMatch(V3ErrorCode code, const string& filename, const string& contents,
                               const string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

void V3Control::addInline(FileLine* fl, const string& module, const string& ftask, bool on) {
    ប្រសិនបើ (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } បើមិនដូច្នេះ {
        ប្រសិនបើ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } បើមិនដូច្នេះ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

void V3Control::addModulePragma(const string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

void V3Control::addProfileData(FileLine* fl, const string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
void V3Control::addProfileData(FileLine* fl, const string& model, const string& key,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, key, cost);
}

void V3Control::addScopeTraceOn(bool on, const string& scope, ចំនួនគត់ levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

void V3Control::addVarAttr(FileLine* fl, const string& module, const string& ftask,
                           VarSpecKind kind, const string& pattern, VAttrType attr,
                           AstSenTree* sensep) {
    ប្រសិនបើ (sensep) {
        FileLine* const flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->deleteTree(), sensep);
        // Used to be only accepted on public_flat_rw
        ប្រសិនបើ (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ត្រឡប់;
        }
    }

    ប្រសិនបើ (kind != VarSpecKind::VAR) {
        switch (attr) {
        pak_sa VAttrType::VAR_PUBLIC_FLAT:
        pak_sa VAttrType::VAR_PUBLIC_FLAT_RD:
        pak_sa VAttrType::VAR_PUBLIC_FLAT_RW: bha_nga;
        default:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ត្រឡប់;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ប្រសិនបើ (pattern.empty()) {
        ប្រសិនបើ (attr == VAttrType::VAR_PUBLIC) {
            ប្រសិនបើ (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } បើមិនដូច្នេះ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(true);
            }
        } បើមិនដូច្នេះ {
            fl->v3error("missing -var");
        }
    } បើមិនដូច្នេះ {
        ប្រសិនបើ (attr == VAttrType::VAR_FORCEABLE) {
            ប្រសិនបើ (module.empty()) {
                fl->v3error("forceable missing -module");
            } បើមិនដូច្នេះ ប្រសិនបើ (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } បើមិនដូច្នេះ {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } បើមិនដូច្នេះ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ប្រសិនបើ (ftask.empty()) {
                    ប្រសិនបើ (kind == VarSpecKind::PARAM) ត្រឡប់ mod.params().at(pattern);
                    ប្រសិនបើ (kind == VarSpecKind::PORT) ត្រឡប់ mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    ត្រឡប់ mod.vars().at(pattern);
                }
                ប្រសិនបើ (kind == VarSpecKind::PARAM) ត្រឡប់ mod.ftasks().at(ftask).params().at(pattern);
                ប្រសិនបើ (kind == VarSpecKind::PORT) ត្រឡប់ mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                ត្រឡប់ mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

void V3Control::applyCase(AstCase* nodep) {
    const string& filename = nodep->fileline()->filename();
    V3ControlFile* const filep = V3ControlResolver::s().files().resolve(filename);
    ប្រសិនបើ (filep) filep->applyCase(nodep);
}

void V3Control::applyCoverageBlock(AstNodeModule* modulep, AstBegin* nodep) {
    const string& filename = nodep->fileline()->filename();
    V3ControlFile* const filep = V3ControlResolver::s().files().resolve(filename);
    ប្រសិនបើ (filep) filep->applyBlock(nodep);
    const string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* const modp = V3ControlResolver::s().modules().resolve(modname);
    ប្រសិនបើ (modp) modp->applyBlock(nodep);
}
void V3Control::applyCoverageBlock(AstNodeModule* modulep, AstGenBlock* nodep) {
    const string& filename = nodep->fileline()->filename();
    V3ControlFile* const filep = V3ControlResolver::s().files().resolve(filename);
    ប្រសិនបើ (filep) filep->applyBlock(nodep);
    const string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* const modp = V3ControlResolver::s().modules().resolve(modname);
    ប្រសិនបើ (modp) modp->applyBlock(nodep);
}

void V3Control::applyIgnores(FileLine* filelinep) {
    const string& filename = filelinep->filename();
    V3ControlFile* const filep = V3ControlResolver::s().files().resolve(filename);
    ប្រសិនបើ (filep) filep->applyIgnores(filelinep);
}

void V3Control::applyModule(AstNodeModule* modulep) {
    const string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* const modp = V3ControlResolver::s().modules().resolve(modname);
    ប្រសិនបើ (modp) modp->apply(modulep);
}

void V3Control::applyFTask(AstNodeModule* modulep, AstNodeFTask* ftaskp) {
    const string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* const modp = V3ControlResolver::s().modules().resolve(modname);
    ប្រសិនបើ (!modp) ត្រឡប់;
    const V3ControlFTask* const ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ប្រសិនបើ (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
static void resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    const std::string name = varp->prettyDehashOrigOrName();
    ប្រសិនបើ (const V3ControlVar* const vp = resolver.vars().resolve(name)) vp->apply(varp);
    ប្រសិនបើ (varp->isParam()) {
        ប្រសិនបើ (const V3ControlVar* const vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ប្រសិនបើ (varp->isIO()) {
        ប្រសិនបើ (const V3ControlVar* const vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

void V3Control::applyVarAttr(const AstNodeModule* modulep, const AstNodeFTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* const modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ប្រសិនបើ (!modp) ត្រឡប់;
    ប្រសិនបើ (ftaskp) {
        V3ControlFTask* const ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ប្រសិនបើ (!ftp) ត្រឡប់;
        resolveThenApply(*ftp, varp);
    } បើមិនដូច្នេះ {
        resolveThenApply(*modp, varp);
    }
}

ចំនួនគត់ V3Control::getHierWorkers(const string& model) {
    ត្រឡប់ V3ControlResolver::s().getHierWorkers(model);
}
FileLine* V3Control::getHierWorkersFileLine(const string& model) {
    ត្រឡប់ V3ControlResolver::s().getHierWorkersFileLine(model);
}
const V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(const string& module) {
    V3ControlModule* const modp = V3ControlResolver::s().modules().resolve(module);
    ត្រឡប់ modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(const string& hierDpi) {
    ត្រឡប់ V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(const string& model, const string& key) {
    ត្រឡប់ V3ControlResolver::s().getProfileData(model, key);
}
FileLine* V3Control::getProfileDataFileLine() {
    ត្រឡប់ V3ControlResolver::s().getProfileDataFileLine();
}
bool V3Control::getScopeTraceOn(const string& scope) {
    ត្រឡប់ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

void V3Control::contentsPushText(const string& text) { WildcardContents::pushText(text); }

bool V3Control::containsMTaskProfileData() {
    ត្រឡប់ V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    ត្រឡប់ V3ControlResolver::s().getCurrentHierBlockCost();
}

bool V3Control::waive(const FileLine* filelinep, V3ErrorCode code, const string& message) {
    V3ControlFile* const filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ប្រសិនបើ (!filep) ត្រឡប់ false;
    ត្រឡប់ filep->waive(code, message);
}

void V3Control::selfTest() { VIntervalTree::selfTest(); }
