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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3Control.h"

#অন্তর্ভাৱ "V3InstrCount.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3String.h"

#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
শ্ৰেণী V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ধ্রুৱ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ধ্রুৱ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

সার্ৱজনিক:
    V3ControlWildcardResolver() = স্ৱভাৱ;
    ~V3ControlWildcardResolver() = স্ৱভাৱ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    শূন্য update(ধ্রুৱ V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ধ্রুৱ V3LockGuard lock{m_mutex};
        ধ্রুৱ V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        বাবে (ধ্রুৱ auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ধ্রুৱ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুৱ V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ঘূৰাই m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ধ্রুৱ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুৱ V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ধ্রুৱ auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        যদি (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ধ্রুৱ string slashName = V3Os::filenameSlashPath(name);
            বাবে (ধ্রুৱ auto& patEnt : m_mapPatterns) {
                যদি (VString::wildmatch(slashName, patEnt.first)) {
                    যদি (!entryr) entryr.reset(নূতন T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            যদি (entryr) entryr->updateFinalize();
        }
        ঘূৰাই entryr.get();
    }
};

// List of attributes for variables
শ্ৰেণী V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
সার্ৱজনিক:
    // Add new attribugte
    শূন্য add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    শূন্য update(ধ্রুৱ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.ঢোকাও(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    শূন্য updateFinalize() {}
    // Apply all attributes to the variable
    শূন্য apply(AstVar* varp) ধ্রুৱ {
        বাবে (ধ্রুৱ VAttrType attr : m_attrs) {
            varp->addAttrsp(নূতন AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

শ্ৰেণী WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ধ্রুৱ std::string, তার্কিক> m_mapPatterns;  // Pattern match results
    std::deque<string> m_সাৰিগুলি;  // Source text lines

    // METHODS
    স্থির WildcardContents& s() {  // Singleton
        স্থির WildcardContents s_s;
        ঘূৰাই s_s;
    }
    শূন্য clearCacheImp() { m_mapPatterns.clear(); }
    শূন্য ঠেলো_Text_Imp(ধ্রুৱ string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ধ্রুৱ string leftover = m_সাৰিগুলি.back() + text;
        m_সাৰিগুলি.বেৰ_কৰো_back();

        // Insert line-by-line
        string::size_type সাৰি_start = 0;
        যেতিয়ালৈ (সঁচা) {
            ধ্রুৱ string::size_type সাৰি_end = leftover.find('\n', সাৰি_start);
            যদি (সাৰি_end != string::npos) {
                ধ্রুৱ string oneline(leftover, সাৰি_start, সাৰি_end - সাৰি_start + 1);
                যদি (oneline.size() > 1) m_সাৰিগুলি.ঠেলো_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_সাৰিগুলি.size() - 1) << "]: " << oneline);
                সাৰি_start = সাৰি_end + 1;
            } অন্যথা {
                ভঙ্গ;
            }
        }
        // Keep leftover for next time
        m_সাৰিগুলি.emplace_back(string(leftover, সাৰি_start));  // Might be ""
        clearCacheImp();
    }

    তার্কিক resolveUncachedImp(ধ্রুৱ string& contentsRegexp) {
        বাবে (ধ্রুৱ string& i : m_সাৰিগুলি) {
            যদি (VString::wildmatch(i, contentsRegexp)) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    তার্কিক resolveCachedImp(ধ্রুৱ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        যদি (contentsRegexp.empty()) ঘূৰাই সঁচা;
        ধ্রুৱ auto pair = m_mapPatterns.emplace(contentsRegexp, মিছা);
        তার্কিক& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        যদি (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        ঘূৰাই entryr;
    }

সার্ৱজনিক:
    WildcardContents() {
        m_সাৰিগুলি.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = স্ৱভাৱ;
    // Return true iff contentsRegexp in parsed contents
    স্থির তার্কিক resolve(ধ্রুৱ string& contentsRegexp) {
        ঘূৰাই s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    স্থির শূন্য ঠেলো_Text(ধ্রুৱ string& text) { s().ঠেলো_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

শ্ৰেণী V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    তার্কিক m_noinline = মিছা;  // Don't inline function/task
    তার্কিক m_public = মিছা;  // Public function/task

সার্ৱজনিক:
    V3ControlFTask() = স্ৱভাৱ;
    শূন্য update(ধ্রুৱ V3ControlFTask& f) {
        // Don't overwrite true with false
        যদি (f.m_noinline) m_noinline = সঁচা;
        যদি (f.m_public) m_public = সঁচা;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    শূন্য updateFinalize() {}

    V3ControlVarResolver& params() { ঘূৰাই m_params; }
    V3ControlVarResolver& ports() { ঘূৰাই m_ports; }
    V3ControlVarResolver& vars() { ঘূৰাই m_vars; }

    শূন্য setNoInline(তার্কিক set) { m_noinline = set; }
    শূন্য setPublic(তার্কিক set) { m_public = set; }

    শূন্য apply(Ast_গ্ৰন্থি_FTask* ftaskp) ধ্রুৱ {
        যদি (m_noinline)
            ftaskp->addStmtsp(নূতন AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        যদি (m_public)
            ftaskp->addStmtsp(নূতন AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

শ্ৰেণী V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    তার্কিক m_hasFsmRegisterWrapper = মিছা;  // Whether descriptor has been set
    তার্কিক m_inline = মিছা;  // Whether to force the inline
    তার্কিক m_inline_মান = মিছা;  // The inline value (on/off)

সার্ৱজনিক:
    V3ControlModule() = স্ৱভাৱ;

    শূন্য update(ধ্রুৱ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        যদি (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = সঁচা;
        }
        বাবে (ধ্রুৱ string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.ঢোকাও(i);
        যদি (!m_inline) {
            m_inline = m.m_inline;
            m_inline_মান = m.m_inline_মান;
        }
        বাবে (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ঢোকাও(*it);
        }
    }
    শূন্য updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ঘূৰাই m_tasks; }
    V3ControlVarResolver& params() { ঘূৰাই m_params; }
    V3ControlVarResolver& ports() { ঘূৰাই m_ports; }
    V3ControlVarResolver& vars() { ঘূৰাই m_vars; }

    শূন্য addCoverageBlockOff(ধ্রুৱ string& name) { m_coverageOffBlocks.ঢোকাও(name); }
    শূন্য setInline(তার্কিক set) {
        m_inline = সঁচা;
        m_inline_মান = set;
    }
    শূন্য setFsmRegisterWrapper(File_সাৰি* fl, ধ্রুৱ V3Control::FsmRegisterWrapper& desc) {
        যদি (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_গ্ৰন্থি::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = সঁচা;
    }
    ধ্রুৱ V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ধ্রুৱ {
        ঘূৰাই m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    শূন্য addModulePragma(VPragmaType pragma) { m_modPragmas.ঢোকাও(pragma); }

    শূন্য apply(Ast_গ্ৰন্থি_Module* modp) {
        যদি (m_inline) {
            ধ্রুৱ VPragmaType type
                = m_inline_মান ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_গ্ৰন্থি* ধ্রুৱ nodep = নূতন AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        বাবে (ধ্রুৱ auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            যদি (itr == VPragmaType::HIER_PARAMS) modp->hierParams(সঁচা);
            Ast_গ্ৰন্থি* ধ্রুৱ nodep = নূতন AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    শূন্য applyBlock(AstGenBlock* nodep) {
        ধ্রুৱ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        যদি (!nodep->unnamed()) {
            বাবে (ধ্রুৱ string& i : m_coverageOffBlocks) {
                যদি (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(নূতন AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    শূন্য applyBlock(Ast_গ্ৰন্থি_Block* nodep) {
        ধ্রুৱ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        যদি (!nodep->unnamed()) {
            বাবে (ধ্রুৱ string& i : m_coverageOffBlocks) {
                যদি (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(নূতন AstStmtPragma{flp, নূতন AstPragma{flp, pragma}});
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
শ্ৰেণী V3_Control_Ignores_সাৰি final {
সার্ৱজনিক:
    ধ্রুৱ পূর্ণাঙ্ক m_সাৰি_Min;  // Minimum line number to make change at
    ধ্রুৱ পূর্ণাঙ্ক m_সাৰি_Max;  // Maximum line number to make change at (inclusive)
    ধ্রুৱ V3ErrorCode m_code;  // Error code
    ধ্রুৱ তার্কিক m_on;  // True to enable message
    V3_Control_Ignores_সাৰি(V3ErrorCode code, পূর্ণাঙ্ক linemin, পূর্ণাঙ্ক linemax, তার্কিক on)
        : m_সাৰি_Min{linemin}
        , m_সাৰি_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_সাৰি() = স্ৱভাৱ;
    তার্কিক every_সাৰি() ধ্রুৱ { ঘূৰাই m_সাৰি_Min == 0 && m_সাৰি_Max == 0; }
    তার্কিক inRange(পূর্ণাঙ্ক lineno) ধ্রুৱ {
        ঘূৰাই (lineno >= m_সাৰি_Min && lineno <= m_সাৰি_Max) || every_সাৰি();
    }
};
std::ostream& operator<<(std::ostream& os, ধ্রুৱ V3_Control_Ignores_সাৰি& rhs) {
    ঘূৰাই os << rhs.m_সাৰি_Min << "-" << rhs.m_সাৰি_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
স্থির std::vector<V3_Control_Ignores_সাৰি> control_Ign_সাৰিগুলি;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

শ্ৰেণী VInterval_বৃক্ষ final {
    সংরচনা Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        পূর্ণাঙ্ক m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(পূর্ণাঙ্ক center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_সাৰিগুলি;  // All-line disables

গুপ্ত:
    std::unique_ptr<Entry> build_বৃক্ষ(ধ্রুৱ IgnIndices& points) {
        যদি (points.empty()) ঘূৰাই nullptr;

        পূর্ণাঙ্ক minval = std::numeric_limits<পূর্ণাঙ্ক>::max();
        পূর্ণাঙ্ক maxval = std::numeric_limits<পূর্ণাঙ্ক>::min();
        বাবে (ধ্রুৱ auto& it : points) {
            ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
            যদি (cign.every_সাৰি()) {
                m_every_সাৰিগুলি.emplace_back(it);
                অনুৱর্ত;
            }
            minval = std::min(minval, cign.m_সাৰি_Min);
            maxval = std::max(maxval, cign.m_সাৰি_Max);
        }

        ধ্রুৱ পূর্ণাঙ্ক center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(নূতন Entry{center});

        IgnIndices বাম_Points;
        IgnIndices ডান_Points;
        বাবে (ধ্রুৱ auto& it : points) {
            ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
            যদি (cign.every_সাৰি()) অনুৱর্ত;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            যদি (cign.m_সাৰি_Max < center)
                বাম_Points.emplace_back(it);
            অন্যথা যদি (cign.m_সাৰি_Min > center)
                ডান_Points.emplace_back(it);
            অন্যথা {
                entp->m_byMin.ঠেলো_back(it);
                entp->m_byMax.ঠেলো_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            ঘূৰাই control_Ign_সাৰিগুলি[a].m_সাৰি_Min < control_Ign_সাৰিগুলি[b].m_সাৰি_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            ঘূৰাই control_Ign_সাৰিগুলি[a].m_সাৰি_Max > control_Ign_সাৰিগুলি[b].m_সাৰি_Max;
        });

        entp->m_leftp = build_বৃক্ষ(বাম_Points);
        entp->m_rightp = build_বৃক্ষ(ডান_Points);
        ঘূৰাই entp;
    }

    শূন্য find_বৃক্ষ(পূর্ণাঙ্ক lineno, Entry* entp, IgnIndices& resultsr, পূর্ণাঙ্ক& nextChanger) ধ্রুৱ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        যদি (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            বাবে (ধ্রুৱ auto& it : entp->m_byMin) {
                ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
                যদি (cign.m_সাৰি_Min <= lineno) {
                    resultsr.emplace_back(it);
                } অন্যথা {
                    nextChanger = std::min(nextChanger, cign.m_সাৰি_Min);
                    ভঙ্গ;
                }
            }
            যদি (entp->m_leftp) find_বৃক্ষ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } অন্যথা যদি (lineno > entp->m_center) {
            বাবে (ধ্রুৱ auto& it : entp->m_byMax) {
                ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
                যদি (cign.m_সাৰি_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_সাৰি_Max + 1);
                    resultsr.emplace_back(it);
                } অন্যথা {
                    ভঙ্গ;
                }
            }
            যদি (entp->m_rightp) find_বৃক্ষ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } অন্যথা {
            বাবে (ধ্রুৱ auto& it : entp->m_byMin) {
                ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
                resultsr.emplace_back(it);
                যদি (cign.m_সাৰি_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_সাৰি_Max + 1);
            }
            যদি (entp->m_leftp) find_বৃক্ষ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            যদি (entp->m_rightp) find_বৃক্ষ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    VInterval_বৃক্ষ() = স্ৱভাৱ;
    ~VInterval_বৃক্ষ() = স্ৱভাৱ;
    // METHODS
    শূন্য clear() { m_rootp = nullptr; }
    শূন্য build(ধ্রুৱ IgnIndices& points) {
        clear();
        m_rootp = build_বৃক্ষ(points);
    }
    শূন্য find(পূর্ণাঙ্ক lineno, IgnIndices& resultsr, পূর্ণাঙ্ক& nextChanger) ধ্রুৱ {
        resultsr = m_every_সাৰিগুলি;
        nextChanger = std::numeric_limits<পূর্ণাঙ্ক>::max();

        যদি (m_rootp) find_বৃক্ষ(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    স্থির শূন্য selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        বাবে (ধ্রুৱ auto& it : points) {
            control_Ign_সাৰিগুলি.emplace_back(
                V3_Control_Ignores_সাৰি{V3ErrorCode::I_LINT, it.first, it.second, সঁচা});
            data.emplace_back(static_cast<uint32_t>(control_Ign_সাৰিগুলি.size() - 1));
        }
        VInterval_বৃক্ষ বৃক্ষ;
        বৃক্ষ.build(data);

        IgnIndices results;
        পূর্ণাঙ্ক nextChange = 0;
        বৃক্ষ.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        বৃক্ষ.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        বৃক্ষ.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        বৃক্ষ.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        বৃক্ষ.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        বৃক্ষ.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        বৃক্ষ.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        বৃক্ষ.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<পূর্ণাঙ্ক>::max());
        //
        points = {{0, 0}};
        বাবে (ধ্রুৱ auto& it : points) {
            control_Ign_সাৰিগুলি.emplace_back(
                V3_Control_Ignores_সাৰি{V3ErrorCode::I_LINT, it.first, it.second, সঁচা});
            data.emplace_back(static_cast<uint32_t>(control_Ign_সাৰিগুলি.size() - 1));
        }
        বৃক্ষ.build(data);
        //
        বৃক্ষ.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_সাৰি_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

শ্ৰেণী WaiverSetting final {
সার্ৱজনিক:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ধ্রুৱ string& contents, ধ্রুৱ string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = স্ৱভাৱ;
    WaiverSetting& operator=(ধ্রুৱ WaiverSetting& rhs) = স্ৱভাৱ;
};

// File entity
শ্ৰেণী V3ControlFile final {
    using সাৰি_Attr_Map = std::map<পূর্ণাঙ্ক, V3_Control_সাৰি_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    সাৰি_Attr_Map m_সাৰি_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_বৃক্ষ m_interval_বৃক্ষ;  // Tree of indices to IgnLines

    সংরচনা {
        পূর্ণাঙ্ক filenameno = -1;  // Last filename
        পূর্ণাঙ্ক lineno = -1;  // Last linenumber
        পূর্ণাঙ্ক nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    তার্কিক সাৰি_Match(পূর্ণাঙ্ক lineno, VPragmaType type) {
        যদি (m_সাৰি_Attrs.find(0) != m_সাৰি_Attrs.end() && m_সাৰি_Attrs[0][type]) ঘূৰাই সঁচা;
        যদি (m_সাৰি_Attrs.find(lineno) == m_সাৰি_Attrs.end()) ঘূৰাই মিছা;
        ঘূৰাই m_সাৰি_Attrs[lineno][type];
    }

সার্ৱজনিক:
    V3ControlFile() = স্ৱভাৱ;
    শূন্য update(ধ্রুৱ V3ControlFile& file) {
        // Copy in all attributes and waivers
        বাবে (ধ্রুৱ auto& itr : file.m_সাৰি_Attrs) m_সাৰি_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.ঢোকাও(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.ঢোকাও(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    শূন্য updateFinalize() { m_interval_বৃক্ষ.build(m_ignIndices); }
    শূন্য add_সাৰি_Attribute(পূর্ণাঙ্ক lineno, VPragmaType attr) { m_সাৰি_Attrs[lineno].set(attr); }
    শূন্য addIgnore(V3ErrorCode code, পূর্ণাঙ্ক min, পূর্ণাঙ্ক max, তার্কিক on) {
        control_Ign_সাৰিগুলি.emplace_back(V3_Control_Ignores_সাৰি{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_সাৰিগুলি.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    শূন্য addIgnoreMatch(V3ErrorCode code, ধ্রুৱ string& contents, ধ্রুৱ string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        যদি (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    শূন্য applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ধ্রুৱ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        যদি (সাৰি_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(নূতন AstPragma{nodep->fileline(), pragma});
        }
    }
    শূন্য applyBlock(Ast_গ্ৰন্থি_Block* nodep) {
        // Apply to block at this line
        ধ্রুৱ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        যদি (সাৰি_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(নূতন AstStmtPragma{flp, নূতন AstPragma{flp, pragma}});
        }
    }
    শূন্য applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ধ্রুৱ পূর্ণাঙ্ক lineno = nodep->fileline()->lineno();
        যদি (সাৰি_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(সঁচা);
        যদি (সাৰি_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(সঁচা);
    }
    শূন্য applyIgnores(File_সাৰি* filelinep) {
        // HOT routine, called each parsed token line of this filename
        যদি (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            ঘূৰাই;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        যদি (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ঘূৰাই;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<পূর্ণাঙ্ক>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_বৃক্ষ.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        বাবে (ধ্রুৱ auto& it : results) {
            ধ্রুৱ V3_Control_Ignores_সাৰি& cign = control_Ign_সাৰিগুলি[it];
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
    তার্কিক waive(V3ErrorCode code, ধ্রুৱ string& message) {
        যদি (code.hardError()) ঘূৰাই মিছা;
        বাবে (ধ্রুৱ auto& itr : m_waivers) {
            যদি ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

শ্ৰেণী V3ControlScopeTraceEntry final {
সার্ৱজনিক:
    ধ্রুৱ string m_scope;  // Scope or regexp to match
    ধ্রুৱ তার্কিক m_on;  // True to enable message
    পূর্ণাঙ্ক m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ধ্রুৱ string& scope, তার্কিক on, পূর্ণাঙ্ক levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = স্ৱভাৱ;
    তার্কিক operator<(ধ্রুৱ V3ControlScopeTraceEntry& other) ধ্রুৱ {
        যদি (m_on < other.m_on) ঘূৰাই সঁচা;
        যদি (m_on > other.m_on) ঘূৰাই মিছা;
        যদি (m_levels < other.m_levels) ঘূৰাই সঁচা;
        যদি (m_levels > other.m_levels) ঘূৰাই মিছা;
        ঘূৰাই m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
শ্ৰেণী V3ControlScopeTraceEntryMatch final {
সার্ৱজনিক:
    ধ্রুৱ V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ধ্রুৱ string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ধ্রুৱ V3ControlScopeTraceEntry* entryp, ধ্রুৱ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    তার্কিক operator<(ধ্রুৱ V3ControlScopeTraceEntryMatch& other) ধ্রুৱ {
        যদি (m_entryp < other.m_entryp) ঘূৰাই সঁচা;
        যদি (m_entryp > other.m_entryp) ঘূৰাই মিছা;
        ঘূৰাই m_scopepart < other.m_scopepart;
    }
};

শ্ৰেণী V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, তার্কিক> m_matchCache;  // Matching entries for speed

সার্ৱজনিক:
    শূন্য addScopeTraceOn(তার্কিক on, ধ্রুৱ string& scope, পূর্ণাঙ্ক levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    তার্কিক getEntryMatch(ধ্রুৱ V3ControlScopeTraceEntry* entp, ধ্রুৱ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ধ্রুৱ V3ControlScopeTraceEntryMatch চাবি{entp, scopepart};
        ধ্রুৱ auto pair = m_matchCache.emplace(চাবি, মিছা);
        যদি (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        ঘূৰাই pair.first->second;
    }

    তার্কিক getScopeTraceOn(ধ্রুৱ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        পূর্ণাঙ্ক maxLevel = 1;
        বাবে (ধ্রুৱ auto& ch : scope) {
            যদি (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        তার্কিক enabled = সঁচা;
        বাবে (ধ্রুৱ auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            পূর্ণাঙ্ক partLevel = 1;
            বাবে (string::size_type partEnd = 0; সঁচা;) {
                partEnd = scope.find('.', partEnd + 1);
                যদি (partEnd == string::npos) partEnd = scope.length();
                ধ্রুৱ std::string scopepart = scope.substr(0, partEnd);
                যদি (getEntryMatch(&ent, scopepart)) {
                    ধ্রুৱ তার্কিক levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    যদি (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ভঙ্গ;
                }
                যদি (partEnd == scope.length()) ভঙ্গ;
                ++partLevel;
            }
        }
        ঘূৰাই enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

শ্ৰেণী V3ControlResolverHierWorkerEntry final {
    ধ্রুৱ পূর্ণাঙ্ক m_workers;  // Number of workers requested for the hierarchical block
    File_সাৰি* ধ্রুৱ m_flp;  // Location the worker count was specified at

সার্ৱজনিক:
    explicit V3ControlResolverHierWorkerEntry(পূর্ণাঙ্ক workers, File_সাৰি* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    পূর্ণাঙ্ক workers() ধ্রুৱ { ঘূৰাই m_workers; }
    File_সাৰি* flp() ধ্রুৱ { ঘূৰাই m_flp; }
};

শ্ৰেণী V3ControlResolver final {
    গণনা ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_সাৰি* m_profile_File_সাৰি = nullptr;  // Location profile_data was read from

    V3ControlResolver() = স্ৱভাৱ;
    ~V3ControlResolver() = স্ৱভাৱ;

সার্ৱজনিক:
    স্থির V3ControlResolver& s() {
        স্থির V3ControlResolver s_singleton;
        ঘূৰাই s_singleton;
    }
    V3ControlModuleResolver& modules() { ঘূৰাই m_modules; }
    V3ControlFileResolver& files() { ঘূৰাই m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ঘূৰাই m_scopeTraces; }

    শূন্য addProfileData(File_সাৰি* fl, ধ্রুৱ string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    শূন্য addProfileData(File_সাৰি* fl, ধ্রুৱ string& model, ধ্রুৱ string& চাবি, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        যদি (!m_profile_File_সাৰি) m_profile_File_সাৰি = fl;
        যদি (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        যদি (mode == MTASK) {
            m_profileData[model][চাবি] += cost;
        } অন্যথা যদি (mode == HIER_DPI) {
            m_profileData[model][চাবি] = std::max(m_profileData[model][চাবি], cost);
        }
        m_mode |= mode;
    }
    তার্কিক containsMTaskProfileData() ধ্রুৱ { ঘূৰাই m_mode & MTASK; }
    uint64_t getProfileData(ধ্রুৱ string& hierDpi) ধ্রুৱ {
        // Empty key for hierarchical DPI wrapper costs.
        ঘূৰাই getProfileData(hierDpi, "");
    }
    শূন্য addHierWorkers(File_সাৰি* flp, ধ্রুৱ string& model, পূর্ণাঙ্ক workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    পূর্ণাঙ্ক getHierWorkers(ধ্রুৱ string& model) ধ্রুৱ {
        ধ্রুৱ auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        ঘূৰাই mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_সাৰি* get_Hier_Workers_File_সাৰি(ধ্রুৱ string& model) ধ্রুৱ {
        ধ্রুৱ auto mit = m_hierWorkers.find(model);
        ঘূৰাই mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ধ্রুৱ string& model, ধ্রুৱ string& চাবি) ধ্রুৱ {
        ধ্রুৱ auto mit = m_profileData.find(model);
        যদি (mit == m_profileData.cend()) ঘূৰাই 0;
        ধ্রুৱ auto it = mit->second.find(চাবি);
        যদি (it == mit->second.cend()) ঘূৰাই 0;
        ঘূৰাই it->second;
    }
    File_সাৰি* get_Profile_Data_File_সাৰি() ধ্রুৱ { ঘূৰাই m_profile_File_সাৰি; }  // Maybe null
    স্থির uint64_t getCurrentHierBlockCost() {
        যদি (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            ঘূৰাই cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
        uint64_t cost = 0;
        বাবে (পূর্ণাঙ্ক i = 0; i < VEval::_ENUM_END; ++i) {
            ধ্রুৱ VEval eval{i};
            যদি (eval.slow()) অনুৱর্ত;
            cost += V3_Instr_গুনতি::গুনতি(netlistp->evalFuncp(eval), মিছা);
        }
        UINFO(9, "Evaluating cost: " << cost);
        ঘূৰাই cost;
    }
};

//######################################################################
// V3Control

শূন্য V3Control::addCaseFull(ধ্রুৱ string& filename, পূর্ণাঙ্ক lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সাৰি_Attribute(lineno, VPragmaType::FULL_CASE);
}
শূন্য V3Control::addCaseParallel(ধ্রুৱ string& filename, পূর্ণাঙ্ক lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সাৰি_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
শূন্য V3Control::addCoverageBlockOff(ধ্রুৱ string& filename, পূর্ণাঙ্ক lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সাৰি_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
শূন্য V3Control::addCoverageBlockOff(ধ্রুৱ string& module, ধ্রুৱ string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

শূন্য V3Control::addHierWorkers(File_সাৰি* fl, ধ্রুৱ string& model, পূর্ণাঙ্ক workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

শূন্য V3Control::addFsmRegisterWrapper(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& d,
                                      ধ্রুৱ string& q, ধ্রুৱ string& clock, ধ্রুৱ string& reset,
                                      ধ্রুৱ string& reset_মান) {
    string missing;
    যদি (module.empty()) missing += "-module";
    যদি (d.empty()) missing = VString::dot(missing, ", ", "-d");
    যদি (q.empty()) missing = VString::dot(missing, ", ", "-q");
    যদি (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    যদি (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ঘূৰাই;
    }

    ধ্রুৱ FsmRegisterWrapper desc{module, d, q, clock, reset, reset_মান};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

শূন্য V3Control::addIgnore(V3ErrorCode code, তার্কিক on, ধ্রুৱ string& filename, পূর্ণাঙ্ক min, পূর্ণাঙ্ক max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    যদি (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_সাৰি::globalWarnOff(code, !on);
    } অন্যথা {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
শূন্য V3Control::addIgnoreMatch(V3ErrorCode code, ধ্রুৱ string& filename, ধ্রুৱ string& contents,
                               ধ্রুৱ string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

শূন্য V3Control::addInline(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& ftask, তার্কিক on) {
    যদি (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } অন্যথা {
        যদি (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } অন্যথা {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

শূন্য V3Control::addModulePragma(ধ্রুৱ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

শূন্য V3Control::addProfileData(File_সাৰি* fl, ধ্রুৱ string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
শূন্য V3Control::addProfileData(File_সাৰি* fl, ধ্রুৱ string& model, ধ্রুৱ string& চাবি,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, চাবি, cost);
}

শূন্য V3Control::addScopeTraceOn(তার্কিক on, ধ্রুৱ string& scope, পূর্ণাঙ্ক levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

শূন্য V3Control::addVarAttr(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& ftask,
                           VarSpecKind kind, ধ্রুৱ string& pattern, VAttrType attr,
                           Ast_Sen_বৃক্ষ* sensep) {
    যদি (sensep) {
        File_সাৰি* ধ্রুৱ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_বৃক্ষ(), sensep);
        // Used to be only accepted on public_flat_rw
        যদি (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ঘূৰাই;
        }
    }

    যদি (kind != VarSpecKind::VAR) {
        ৱিকল্প (attr) {
        পক্ষ VAttrType::VAR_PUBLIC_FLAT:
        পক্ষ VAttrType::VAR_PUBLIC_FLAT_RD:
        পক্ষ VAttrType::VAR_PUBLIC_FLAT_RW: ভঙ্গ;
        স্ৱভাৱ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ঘূৰাই;
        }
    }

    // Semantics: Most of the attributes operate on signals
    যদি (pattern.empty()) {
        যদি (attr == VAttrType::VAR_PUBLIC) {
            যদি (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } অন্যথা {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(সঁচা);
            }
        } অন্যথা {
            fl->v3error("missing -var");
        }
    } অন্যথা {
        যদি (attr == VAttrType::VAR_FORCEABLE) {
            যদি (module.empty()) {
                fl->v3error("forceable missing -module");
            } অন্যথা যদি (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } অন্যথা {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } অন্যথা {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                যদি (ftask.empty()) {
                    যদি (kind == VarSpecKind::PARAM) ঘূৰাই mod.params().at(pattern);
                    যদি (kind == VarSpecKind::PORT) ঘূৰাই mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    ঘূৰাই mod.vars().at(pattern);
                }
                যদি (kind == VarSpecKind::PARAM) ঘূৰাই mod.ftasks().at(ftask).params().at(pattern);
                যদি (kind == VarSpecKind::PORT) ঘূৰাই mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                ঘূৰাই mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

শূন্য V3Control::applyCase(AstCase* nodep) {
    ধ্রুৱ string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুৱ filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyCase(nodep);
}

শূন্য V3Control::applyCoverageBlock(Ast_গ্ৰন্থি_Module* modulep, AstBegin* nodep) {
    ধ্রুৱ string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুৱ filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyBlock(nodep);
    ধ্রুৱ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুৱ modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->applyBlock(nodep);
}
শূন্য V3Control::applyCoverageBlock(Ast_গ্ৰন্থি_Module* modulep, AstGenBlock* nodep) {
    ধ্রুৱ string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুৱ filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyBlock(nodep);
    ধ্রুৱ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুৱ modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->applyBlock(nodep);
}

শূন্য V3Control::applyIgnores(File_সাৰি* filelinep) {
    ধ্রুৱ string& filename = filelinep->filename();
    V3ControlFile* ধ্রুৱ filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyIgnores(filelinep);
}

শূন্য V3Control::applyModule(Ast_গ্ৰন্থি_Module* modulep) {
    ধ্রুৱ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুৱ modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->apply(modulep);
}

শূন্য V3Control::applyFTask(Ast_গ্ৰন্থি_Module* modulep, Ast_গ্ৰন্থি_FTask* ftaskp) {
    ধ্রুৱ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুৱ modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (!modp) ঘূৰাই;
    ধ্রুৱ V3ControlFTask* ধ্রুৱ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    যদি (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
স্থির শূন্য resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ধ্রুৱ std::string name = varp->prettyDehashOrigOrName();
    যদি (ধ্রুৱ V3ControlVar* ধ্রুৱ vp = resolver.vars().resolve(name)) vp->apply(varp);
    যদি (varp->isParam()) {
        যদি (ধ্রুৱ V3ControlVar* ধ্রুৱ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    যদি (varp->isIO()) {
        যদি (ধ্রুৱ V3ControlVar* ধ্রুৱ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

শূন্য V3Control::applyVarAttr(ধ্রুৱ Ast_গ্ৰন্থি_Module* modulep, ধ্রুৱ Ast_গ্ৰন্থি_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ধ্রুৱ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    যদি (!modp) ঘূৰাই;
    যদি (ftaskp) {
        V3ControlFTask* ধ্রুৱ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        যদি (!ftp) ঘূৰাই;
        resolveThenApply(*ftp, varp);
    } অন্যথা {
        resolveThenApply(*modp, varp);
    }
}

পূর্ণাঙ্ক V3Control::getHierWorkers(ধ্রুৱ string& model) {
    ঘূৰাই V3ControlResolver::s().getHierWorkers(model);
}
File_সাৰি* V3Control::get_Hier_Workers_File_সাৰি(ধ্রুৱ string& model) {
    ঘূৰাই V3ControlResolver::s().get_Hier_Workers_File_সাৰি(model);
}
ধ্রুৱ V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ধ্রুৱ string& module) {
    V3ControlModule* ধ্রুৱ modp = V3ControlResolver::s().modules().resolve(module);
    ঘূৰাই modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ধ্রুৱ string& hierDpi) {
    ঘূৰাই V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ধ্রুৱ string& model, ধ্রুৱ string& চাবি) {
    ঘূৰাই V3ControlResolver::s().getProfileData(model, চাবি);
}
File_সাৰি* V3Control::get_Profile_Data_File_সাৰি() {
    ঘূৰাই V3ControlResolver::s().get_Profile_Data_File_সাৰি();
}
তার্কিক V3Control::getScopeTraceOn(ধ্রুৱ string& scope) {
    ঘূৰাই V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

শূন্য V3Control::contents_ঠেলো_Text(ধ্রুৱ string& text) { WildcardContents::ঠেলো_Text(text); }

তার্কিক V3Control::containsMTaskProfileData() {
    ঘূৰাই V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    ঘূৰাই V3ControlResolver::s().getCurrentHierBlockCost();
}

তার্কিক V3Control::waive(ধ্রুৱ File_সাৰি* filelinep, V3ErrorCode code, ধ্রুৱ string& message) {
    V3ControlFile* ধ্রুৱ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    যদি (!filep) ঘূৰাই মিছা;
    ঘূৰাই filep->waive(code, message);
}

শূন্য V3Control::selfTest() { VInterval_বৃক্ষ::selfTest(); }
