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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3Control.h"

#অন্তর্ভাব "V3InstrCount.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <memory>
#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
শ্রেণী V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ধ্রুবক std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ধ্রুবক std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

সর্বজনীন:
    V3ControlWildcardResolver() = পূর্বনির্ধারিত;
    ~V3ControlWildcardResolver() = পূর্বনির্ধারিত;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    শূন্য update(ধ্রুবক V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ধ্রুবক V3LockGuard lock{m_mutex};
        ধ্রুবক V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        জন্য (ধ্রুবক auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ধ্রুবক string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুবক V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ফেরত m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ধ্রুবক string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুবক V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ধ্রুবক auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        যদি (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ধ্রুবক string slashName = V3Os::filenameSlashPath(name);
            জন্য (ধ্রুবক auto& patEnt : m_mapPatterns) {
                যদি (VString::wildmatch(slashName, patEnt.first)) {
                    যদি (!entryr) entryr.reset(নতুন T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            যদি (entryr) entryr->updateFinalize();
        }
        ফেরত entryr.get();
    }
};

// List of attributes for variables
শ্রেণী V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
সর্বজনীন:
    // Add new attribugte
    শূন্য add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    শূন্য update(ধ্রুবক V3ControlVar& other) {
        m_attrs.reserve(m_attrs.আকার_2() + other.m_attrs.আকার_2());
        m_attrs.ঢোকাও(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    শূন্য updateFinalize() {}
    // Apply all attributes to the variable
    শূন্য apply(AstVar* varp) ধ্রুবক {
        জন্য (ধ্রুবক VAttrType attr : m_attrs) {
            varp->addAttrsp(নতুন AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

শ্রেণী WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ধ্রুবক std::string, যৌক্তিক> m_mapPatterns;  // Pattern match results
    std::deque<string> m_সারিগুলি;  // Source text lines

    // METHODS
    স্থির WildcardContents& s() {  // Singleton
        স্থির WildcardContents s_s;
        ফেরত s_s;
    }
    শূন্য clearCacheImp() { m_mapPatterns.clear(); }
    শূন্য ঠেলো_Text_Imp(ধ্রুবক string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ধ্রুবক string leftover = m_সারিগুলি.back() + text;
        m_সারিগুলি.বের_করো_back();

        // Insert line-by-line
        string::আকার_type সারি_start = 0;
        যখন (সত্য) {
            ধ্রুবক string::আকার_type সারি_end = leftover.find('\n', সারি_start);
            যদি (সারি_end != string::npos) {
                ধ্রুবক string oneline(leftover, সারি_start, সারি_end - সারি_start + 1);
                যদি (oneline.আকার_2() > 1) m_সারিগুলি.ঠেলো_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_সারিগুলি.আকার_2() - 1) << "]: " << oneline);
                সারি_start = সারি_end + 1;
            } অন্যথা {
                বিরতি;
            }
        }
        // Keep leftover for next time
        m_সারিগুলি.emplace_back(string(leftover, সারি_start));  // Might be ""
        clearCacheImp();
    }

    যৌক্তিক resolveUncachedImp(ধ্রুবক string& contentsRegexp) {
        জন্য (ধ্রুবক string& i : m_সারিগুলি) {
            যদি (VString::wildmatch(i, contentsRegexp)) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    যৌক্তিক resolveCachedImp(ধ্রুবক string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        যদি (contentsRegexp.empty()) ফেরত সত্য;
        ধ্রুবক auto pair = m_mapPatterns.emplace(contentsRegexp, মিথ্যা);
        যৌক্তিক& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        যদি (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        ফেরত entryr;
    }

সর্বজনীন:
    WildcardContents() {
        m_সারিগুলি.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = পূর্বনির্ধারিত;
    // Return true iff contentsRegexp in parsed contents
    স্থির যৌক্তিক resolve(ধ্রুবক string& contentsRegexp) {
        ফেরত s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    স্থির শূন্য ঠেলো_Text(ধ্রুবক string& text) { s().ঠেলো_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

শ্রেণী V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    যৌক্তিক m_noinline = মিথ্যা;  // Don't inline function/task
    যৌক্তিক m_public = মিথ্যা;  // Public function/task

সর্বজনীন:
    V3ControlFTask() = পূর্বনির্ধারিত;
    শূন্য update(ধ্রুবক V3ControlFTask& f) {
        // Don't overwrite true with false
        যদি (f.m_noinline) m_noinline = সত্য;
        যদি (f.m_public) m_public = সত্য;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    শূন্য updateFinalize() {}

    V3ControlVarResolver& params() { ফেরত m_params; }
    V3ControlVarResolver& ports() { ফেরত m_ports; }
    V3ControlVarResolver& vars() { ফেরত m_vars; }

    শূন্য setNoInline(যৌক্তিক set) { m_noinline = set; }
    শূন্য setPublic(যৌক্তিক set) { m_public = set; }

    শূন্য apply(Ast_গ্রন্থি_FTask* ftaskp) ধ্রুবক {
        যদি (m_noinline)
            ftaskp->addStmtsp(নতুন AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        যদি (m_public)
            ftaskp->addStmtsp(নতুন AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

শ্রেণী V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    যৌক্তিক m_hasFsmRegisterWrapper = মিথ্যা;  // Whether descriptor has been set
    যৌক্তিক m_inline = মিথ্যা;  // Whether to force the inline
    যৌক্তিক m_inline_মান = মিথ্যা;  // The inline value (on/off)

সর্বজনীন:
    V3ControlModule() = পূর্বনির্ধারিত;

    শূন্য update(ধ্রুবক V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        যদি (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = সত্য;
        }
        জন্য (ধ্রুবক string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.ঢোকাও(i);
        যদি (!m_inline) {
            m_inline = m.m_inline;
            m_inline_মান = m.m_inline_মান;
        }
        জন্য (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ঢোকাও(*it);
        }
    }
    শূন্য updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ফেরত m_tasks; }
    V3ControlVarResolver& params() { ফেরত m_params; }
    V3ControlVarResolver& ports() { ফেরত m_ports; }
    V3ControlVarResolver& vars() { ফেরত m_vars; }

    শূন্য addCoverageBlockOff(ধ্রুবক string& name) { m_coverageOffBlocks.ঢোকাও(name); }
    শূন্য setInline(যৌক্তিক set) {
        m_inline = সত্য;
        m_inline_মান = set;
    }
    শূন্য setFsmRegisterWrapper(File_সারি* fl, ধ্রুবক V3Control::FsmRegisterWrapper& desc) {
        যদি (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_গ্রন্থি::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = সত্য;
    }
    ধ্রুবক V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ধ্রুবক {
        ফেরত m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    শূন্য addModulePragma(VPragmaType pragma) { m_modPragmas.ঢোকাও(pragma); }

    শূন্য apply(Ast_গ্রন্থি_Module* modp) {
        যদি (m_inline) {
            ধ্রুবক VPragmaType type
                = m_inline_মান ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_গ্রন্থি* ধ্রুবক nodep = নতুন AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        জন্য (ধ্রুবক auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            যদি (itr == VPragmaType::HIER_PARAMS) modp->hierParams(সত্য);
            Ast_গ্রন্থি* ধ্রুবক nodep = নতুন AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    শূন্য applyBlock(AstGenBlock* nodep) {
        ধ্রুবক VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        যদি (!nodep->unnamed()) {
            জন্য (ধ্রুবক string& i : m_coverageOffBlocks) {
                যদি (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(নতুন AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    শূন্য applyBlock(Ast_গ্রন্থি_Block* nodep) {
        ধ্রুবক VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_সারি* ধ্রুবক flp = nodep->fileline();
        যদি (!nodep->unnamed()) {
            জন্য (ধ্রুবক string& i : m_coverageOffBlocks) {
                যদি (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(নতুন AstStmtPragma{flp, নতুন AstPragma{flp, pragma}});
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
শ্রেণী V3_Control_Ignores_সারি final {
সর্বজনীন:
    ধ্রুবক পূর্ণসংখ্যা m_সারি_Min;  // Minimum line number to make change at
    ধ্রুবক পূর্ণসংখ্যা m_সারি_Max;  // Maximum line number to make change at (inclusive)
    ধ্রুবক V3ErrorCode m_code;  // Error code
    ধ্রুবক যৌক্তিক m_on;  // True to enable message
    V3_Control_Ignores_সারি(V3ErrorCode code, পূর্ণসংখ্যা linemin, পূর্ণসংখ্যা linemax, যৌক্তিক on)
        : m_সারি_Min{linemin}
        , m_সারি_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_সারি() = পূর্বনির্ধারিত;
    যৌক্তিক every_সারি() ধ্রুবক { ফেরত m_সারি_Min == 0 && m_সারি_Max == 0; }
    যৌক্তিক inRange(পূর্ণসংখ্যা lineno) ধ্রুবক {
        ফেরত (lineno >= m_সারি_Min && lineno <= m_সারি_Max) || every_সারি();
    }
};
std::ostream& operator<<(std::ostream& os, ধ্রুবক V3_Control_Ignores_সারি& rhs) {
    ফেরত os << rhs.m_সারি_Min << "-" << rhs.m_সারি_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
স্থির std::vector<V3_Control_Ignores_সারি> control_Ign_সারিগুলি;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

শ্রেণী VInterval_বৃক্ষ final {
    কাঠামো Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        পূর্ণসংখ্যা m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(পূর্ণসংখ্যা center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_সারিগুলি;  // All-line disables

ব্যক্তিগত:
    std::unique_ptr<Entry> build_বৃক্ষ(ধ্রুবক IgnIndices& points) {
        যদি (points.empty()) ফেরত nullptr;

        পূর্ণসংখ্যা minval = std::numeric_limits<পূর্ণসংখ্যা>::max();
        পূর্ণসংখ্যা maxval = std::numeric_limits<পূর্ণসংখ্যা>::min();
        জন্য (ধ্রুবক auto& it : points) {
            ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
            যদি (cign.every_সারি()) {
                m_every_সারিগুলি.emplace_back(it);
                চালু;
            }
            minval = std::min(minval, cign.m_সারি_Min);
            maxval = std::max(maxval, cign.m_সারি_Max);
        }

        ধ্রুবক পূর্ণসংখ্যা center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(নতুন Entry{center});

        IgnIndices বাম_Points;
        IgnIndices ডান_Points;
        জন্য (ধ্রুবক auto& it : points) {
            ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
            যদি (cign.every_সারি()) চালু;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            যদি (cign.m_সারি_Max < center)
                বাম_Points.emplace_back(it);
            অন্যথা যদি (cign.m_সারি_Min > center)
                ডান_Points.emplace_back(it);
            অন্যথা {
                entp->m_byMin.ঠেলো_back(it);
                entp->m_byMax.ঠেলো_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            ফেরত control_Ign_সারিগুলি[a].m_সারি_Min < control_Ign_সারিগুলি[b].m_সারি_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            ফেরত control_Ign_সারিগুলি[a].m_সারি_Max > control_Ign_সারিগুলি[b].m_সারি_Max;
        });

        entp->m_leftp = build_বৃক্ষ(বাম_Points);
        entp->m_rightp = build_বৃক্ষ(ডান_Points);
        ফেরত entp;
    }

    শূন্য find_বৃক্ষ(পূর্ণসংখ্যা lineno, Entry* entp, IgnIndices& resultsr, পূর্ণসংখ্যা& পরবর্তী_Changer) ধ্রুবক {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        যদি (lineno < entp->m_center) {
            পরবর্তী_Changer = std::min(পরবর্তী_Changer, entp->m_center);
            জন্য (ধ্রুবক auto& it : entp->m_byMin) {
                ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
                যদি (cign.m_সারি_Min <= lineno) {
                    resultsr.emplace_back(it);
                } অন্যথা {
                    পরবর্তী_Changer = std::min(পরবর্তী_Changer, cign.m_সারি_Min);
                    বিরতি;
                }
            }
            যদি (entp->m_leftp) find_বৃক্ষ(lineno, entp->m_leftp.get(), resultsr, পরবর্তী_Changer);
        } অন্যথা যদি (lineno > entp->m_center) {
            জন্য (ধ্রুবক auto& it : entp->m_byMax) {
                ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
                যদি (cign.m_সারি_Max >= lineno) {
                    পরবর্তী_Changer = std::min(পরবর্তী_Changer, cign.m_সারি_Max + 1);
                    resultsr.emplace_back(it);
                } অন্যথা {
                    বিরতি;
                }
            }
            যদি (entp->m_rightp) find_বৃক্ষ(lineno, entp->m_rightp.get(), resultsr, পরবর্তী_Changer);
        } অন্যথা {
            জন্য (ধ্রুবক auto& it : entp->m_byMin) {
                ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
                resultsr.emplace_back(it);
                যদি (cign.m_সারি_Max >= lineno)
                    পরবর্তী_Changer = std::min(পরবর্তী_Changer, cign.m_সারি_Max + 1);
            }
            যদি (entp->m_leftp) find_বৃক্ষ(lineno, entp->m_leftp.get(), resultsr, পরবর্তী_Changer);
            যদি (entp->m_rightp) find_বৃক্ষ(lineno, entp->m_rightp.get(), resultsr, পরবর্তী_Changer);
        }
    }

সর্বজনীন:
    // CONSTRUCTORS
    VInterval_বৃক্ষ() = পূর্বনির্ধারিত;
    ~VInterval_বৃক্ষ() = পূর্বনির্ধারিত;
    // METHODS
    শূন্য clear() { m_rootp = nullptr; }
    শূন্য build(ধ্রুবক IgnIndices& points) {
        clear();
        m_rootp = build_বৃক্ষ(points);
    }
    শূন্য find(পূর্ণসংখ্যা lineno, IgnIndices& resultsr, পূর্ণসংখ্যা& পরবর্তী_Changer) ধ্রুবক {
        resultsr = m_every_সারিগুলি;
        পরবর্তী_Changer = std::numeric_limits<পূর্ণসংখ্যা>::max();

        যদি (m_rootp) find_বৃক্ষ(lineno, m_rootp.get(), resultsr, পরবর্তী_Changer);
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
        IgnIndices তথ্য;
        std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        জন্য (ধ্রুবক auto& it : points) {
            control_Ign_সারিগুলি.emplace_back(
                V3_Control_Ignores_সারি{V3ErrorCode::I_LINT, it.first, it.second, সত্য});
            তথ্য.emplace_back(static_cast<uint32_t>(control_Ign_সারিগুলি.আকার_2() - 1));
        }
        VInterval_বৃক্ষ বৃক্ষ;
        বৃক্ষ.build(তথ্য);

        IgnIndices results;
        পূর্ণসংখ্যা পরবর্তী_Change = 0;
        বৃক্ষ.find(0, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 0);
        UASSERT_SELFTEST(পরবর্তী_Change, 10);
        বৃক্ষ.find(10, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(পরবর্তী_Change, 11);
        বৃক্ষ.find(11, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(পরবর্তী_Change, 15);  // Center, or would be 20
        বৃক্ষ.find(20, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(পরবর্তী_Change, 21);
        বৃক্ষ.find(21, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(পরবর্তী_Change, 25);  // Center, or would be 30
        বৃক্ষ.find(30, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(পরবর্তী_Change, 31);
        বৃক্ষ.find(40, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(পরবর্তী_Change, 41);
        বৃক্ষ.find(41, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 0);
        UASSERT_SELFTEST(পরবর্তী_Change, std::numeric_limits<পূর্ণসংখ্যা>::max());
        //
        points = {{0, 0}};
        জন্য (ধ্রুবক auto& it : points) {
            control_Ign_সারিগুলি.emplace_back(
                V3_Control_Ignores_সারি{V3ErrorCode::I_LINT, it.first, it.second, সত্য});
            তথ্য.emplace_back(static_cast<uint32_t>(control_Ign_সারিগুলি.আকার_2() - 1));
        }
        বৃক্ষ.build(তথ্য);
        //
        বৃক্ষ.find(50, results, পরবর্তী_Change);
        UASSERT_SELFTEST(results.আকার_2(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_সারি_Attribute = std::bitset<VPragmaType::ENUM_আকার>;

শ্রেণী WaiverSetting final {
সর্বজনীন:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ধ্রুবক string& contents, ধ্রুবক string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = পূর্বনির্ধারিত;
    WaiverSetting& operator=(ধ্রুবক WaiverSetting& rhs) = পূর্বনির্ধারিত;
};

// File entity
শ্রেণী V3ControlFile final {
    using সারি_Attr_Map = std::map<পূর্ণসংখ্যা, V3_Control_সারি_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    সারি_Attr_Map m_সারি_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_বৃক্ষ m_interval_বৃক্ষ;  // Tree of indices to IgnLines

    কাঠামো {
        পূর্ণসংখ্যা filenameno = -1;  // Last filename
        পূর্ণসংখ্যা lineno = -1;  // Last linenumber
        পূর্ণসংখ্যা পরবর্তী_Change = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    যৌক্তিক সারি_Match(পূর্ণসংখ্যা lineno, VPragmaType type) {
        যদি (m_সারি_Attrs.find(0) != m_সারি_Attrs.end() && m_সারি_Attrs[0][type]) ফেরত সত্য;
        যদি (m_সারি_Attrs.find(lineno) == m_সারি_Attrs.end()) ফেরত মিথ্যা;
        ফেরত m_সারি_Attrs[lineno][type];
    }

সর্বজনীন:
    V3ControlFile() = পূর্বনির্ধারিত;
    শূন্য update(ধ্রুবক V3ControlFile& file) {
        // Copy in all attributes and waivers
        জন্য (ধ্রুবক auto& itr : file.m_সারি_Attrs) m_সারি_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.আকার_2() + file.m_waivers.আকার_2());
        m_waivers.ঢোকাও(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.আকার_2() + file.m_ignIndices.আকার_2());
        m_ignIndices.ঢোকাও(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    শূন্য updateFinalize() { m_interval_বৃক্ষ.build(m_ignIndices); }
    শূন্য add_সারি_Attribute(পূর্ণসংখ্যা lineno, VPragmaType attr) { m_সারি_Attrs[lineno].set(attr); }
    শূন্য addIgnore(V3ErrorCode code, পূর্ণসংখ্যা min, পূর্ণসংখ্যা max, যৌক্তিক on) {
        control_Ign_সারিগুলি.emplace_back(V3_Control_Ignores_সারি{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_সারিগুলি.আকার_2() - 1));
        m_lastIgnore.পরবর্তী_Change = -1;
    }
    শূন্য addIgnoreMatch(V3ErrorCode code, ধ্রুবক string& contents, ধ্রুবক string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        যদি (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    শূন্য applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ধ্রুবক VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        যদি (সারি_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(নতুন AstPragma{nodep->fileline(), pragma});
        }
    }
    শূন্য applyBlock(Ast_গ্রন্থি_Block* nodep) {
        // Apply to block at this line
        ধ্রুবক VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_সারি* ধ্রুবক flp = nodep->fileline();
        যদি (সারি_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(নতুন AstStmtPragma{flp, নতুন AstPragma{flp, pragma}});
        }
    }
    শূন্য applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ধ্রুবক পূর্ণসংখ্যা lineno = nodep->fileline()->lineno();
        যদি (সারি_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(সত্য);
        যদি (সারি_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(সত্য);
    }
    শূন্য applyIgnores(File_সারি* filelinep) {
        // HOT routine, called each parsed token line of this filename
        যদি (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            ফেরত;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        যদি (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.পরবর্তী_Change) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ফেরত;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.পরবর্তী_Change = std::numeric_limits<পূর্ণসংখ্যা>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_বৃক্ষ.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.পরবর্তী_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        জন্য (ধ্রুবক auto& it : results) {
            ধ্রুবক V3_Control_Ignores_সারি& cign = control_Ign_সারিগুলি[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.পরবর্তী_Change = std::max(m_lastIgnore.পরবর্তী_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.পরবর্তী_Change
                                       << " Messages: " << bitset.ascii());
    }
    যৌক্তিক waive(V3ErrorCode code, ধ্রুবক string& message) {
        যদি (code.hardError()) ফেরত মিথ্যা;
        জন্য (ধ্রুবক auto& itr : m_waivers) {
            যদি ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                ফেরত সত্য;
            }
        }
        ফেরত মিথ্যা;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

শ্রেণী V3ControlScopeTraceEntry final {
সর্বজনীন:
    ধ্রুবক string m_scope;  // Scope or regexp to match
    ধ্রুবক যৌক্তিক m_on;  // True to enable message
    পূর্ণসংখ্যা m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ধ্রুবক string& scope, যৌক্তিক on, পূর্ণসংখ্যা levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = পূর্বনির্ধারিত;
    যৌক্তিক operator<(ধ্রুবক V3ControlScopeTraceEntry& other) ধ্রুবক {
        যদি (m_on < other.m_on) ফেরত সত্য;
        যদি (m_on > other.m_on) ফেরত মিথ্যা;
        যদি (m_levels < other.m_levels) ফেরত সত্য;
        যদি (m_levels > other.m_levels) ফেরত মিথ্যা;
        ফেরত m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
শ্রেণী V3ControlScopeTraceEntryMatch final {
সর্বজনীন:
    ধ্রুবক V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ধ্রুবক string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ধ্রুবক V3ControlScopeTraceEntry* entryp, ধ্রুবক string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    যৌক্তিক operator<(ধ্রুবক V3ControlScopeTraceEntryMatch& other) ধ্রুবক {
        যদি (m_entryp < other.m_entryp) ফেরত সত্য;
        যদি (m_entryp > other.m_entryp) ফেরত মিথ্যা;
        ফেরত m_scopepart < other.m_scopepart;
    }
};

শ্রেণী V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, যৌক্তিক> m_matchCache;  // Matching entries for speed

সর্বজনীন:
    শূন্য addScopeTraceOn(যৌক্তিক on, ধ্রুবক string& scope, পূর্ণসংখ্যা levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    যৌক্তিক getEntryMatch(ধ্রুবক V3ControlScopeTraceEntry* entp, ধ্রুবক string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ধ্রুবক V3ControlScopeTraceEntryMatch চাবি{entp, scopepart};
        ধ্রুবক auto pair = m_matchCache.emplace(চাবি, মিথ্যা);
        যদি (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        ফেরত pair.first->second;
    }

    যৌক্তিক getScopeTraceOn(ধ্রুবক string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        পূর্ণসংখ্যা maxLevel = 1;
        জন্য (ধ্রুবক auto& ch : scope) {
            যদি (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        যৌক্তিক enabled = সত্য;
        জন্য (ধ্রুবক auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            পূর্ণসংখ্যা partLevel = 1;
            জন্য (string::আকার_type partEnd = 0; সত্য;) {
                partEnd = scope.find('.', partEnd + 1);
                যদি (partEnd == string::npos) partEnd = scope.দৈর্ঘ্য();
                ধ্রুবক std::string scopepart = scope.substr(0, partEnd);
                যদি (getEntryMatch(&ent, scopepart)) {
                    ধ্রুবক যৌক্তিক levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    যদি (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    বিরতি;
                }
                যদি (partEnd == scope.দৈর্ঘ্য()) বিরতি;
                ++partLevel;
            }
        }
        ফেরত enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

শ্রেণী V3ControlResolverHierWorkerEntry final {
    ধ্রুবক পূর্ণসংখ্যা m_workers;  // Number of workers requested for the hierarchical block
    File_সারি* ধ্রুবক m_flp;  // Location the worker count was specified at

সর্বজনীন:
    explicit V3ControlResolverHierWorkerEntry(পূর্ণসংখ্যা workers, File_সারি* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    পূর্ণসংখ্যা workers() ধ্রুবক { ফেরত m_workers; }
    File_সারি* flp() ধ্রুবক { ফেরত m_flp; }
};

শ্রেণী V3ControlResolver final {
    গণনা Profile_তথ্য_Mode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_তথ্য;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_সারি* m_profile_File_সারি = nullptr;  // Location profile_data was read from

    V3ControlResolver() = পূর্বনির্ধারিত;
    ~V3ControlResolver() = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির V3ControlResolver& s() {
        স্থির V3ControlResolver s_singleton;
        ফেরত s_singleton;
    }
    V3ControlModuleResolver& modules() { ফেরত m_modules; }
    V3ControlFileResolver& files() { ফেরত m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ফেরত m_scopeTraces; }

    শূন্য add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        add_Profile_তথ্য(fl, hierDpi, "", cost, HIER_DPI);
    }
    শূন্য add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& model, ধ্রুবক string& চাবি, uint64_t cost,
                        Profile_তথ্য_Mode mode = MTASK) {
        যদি (!m_profile_File_সারি) m_profile_File_সারি = fl;
        যদি (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        যদি (mode == MTASK) {
            m_profile_তথ্য[model][চাবি] += cost;
        } অন্যথা যদি (mode == HIER_DPI) {
            m_profile_তথ্য[model][চাবি] = std::max(m_profile_তথ্য[model][চাবি], cost);
        }
        m_mode |= mode;
    }
    যৌক্তিক contains_MTask_Profile_তথ্য() ধ্রুবক { ফেরত m_mode & MTASK; }
    uint64_t get_Profile_তথ্য(ধ্রুবক string& hierDpi) ধ্রুবক {
        // Empty key for hierarchical DPI wrapper costs.
        ফেরত get_Profile_তথ্য(hierDpi, "");
    }
    শূন্য addHierWorkers(File_সারি* flp, ধ্রুবক string& model, পূর্ণসংখ্যা workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    পূর্ণসংখ্যা getHierWorkers(ধ্রুবক string& model) ধ্রুবক {
        ধ্রুবক auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        ফেরত mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_সারি* get_Hier_Workers_File_সারি(ধ্রুবক string& model) ধ্রুবক {
        ধ্রুবক auto mit = m_hierWorkers.find(model);
        ফেরত mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_তথ্য(ধ্রুবক string& model, ধ্রুবক string& চাবি) ধ্রুবক {
        ধ্রুবক auto mit = m_profile_তথ্য.find(model);
        যদি (mit == m_profile_তথ্য.cend()) ফেরত 0;
        ধ্রুবক auto it = mit->second.find(চাবি);
        যদি (it == mit->second.cend()) ফেরত 0;
        ফেরত it->second;
    }
    File_সারি* get_Profile_তথ্য_File_সারি() ধ্রুবক { ফেরত m_profile_File_সারি; }  // Maybe null
    স্থির uint64_t getCurrentHierBlockCost() {
        যদি (uint64_t cost = V3Control::get_Profile_তথ্য(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            ফেরত cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ধ্রুবক netlistp = v3Global.rootp();
        uint64_t cost = 0;
        জন্য (পূর্ণসংখ্যা i = 0; i < VEval::_ENUM_END; ++i) {
            ধ্রুবক VEval eval{i};
            যদি (eval.slow()) চালু;
            cost += V3_Instr_গুনতি::গুনতি(netlistp->evalFuncp(eval), মিথ্যা);
        }
        UINFO(9, "Evaluating cost: " << cost);
        ফেরত cost;
    }
};

//######################################################################
// V3Control

শূন্য V3Control::addCaseFull(ধ্রুবক string& filename, পূর্ণসংখ্যা lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সারি_Attribute(lineno, VPragmaType::FULL_CASE);
}
শূন্য V3Control::addCaseParallel(ধ্রুবক string& filename, পূর্ণসংখ্যা lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সারি_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
শূন্য V3Control::addCoverageBlockOff(ধ্রুবক string& filename, পূর্ণসংখ্যা lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_সারি_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
শূন্য V3Control::addCoverageBlockOff(ধ্রুবক string& module, ধ্রুবক string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

শূন্য V3Control::addHierWorkers(File_সারি* fl, ধ্রুবক string& model, পূর্ণসংখ্যা workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

শূন্য V3Control::addFsmRegisterWrapper(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& d,
                                      ধ্রুবক string& q, ধ্রুবক string& clock, ধ্রুবক string& reset,
                                      ধ্রুবক string& reset_মান) {
    string missing;
    যদি (module.empty()) missing += "-module";
    যদি (d.empty()) missing = VString::dot(missing, ", ", "-d");
    যদি (q.empty()) missing = VString::dot(missing, ", ", "-q");
    যদি (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    যদি (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ফেরত;
    }

    ধ্রুবক FsmRegisterWrapper desc{module, d, q, clock, reset, reset_মান};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

শূন্য V3Control::addIgnore(V3ErrorCode code, যৌক্তিক on, ধ্রুবক string& filename, পূর্ণসংখ্যা min, পূর্ণসংখ্যা max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    যদি (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_সারি::globalWarnOff(code, !on);
    } অন্যথা {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
শূন্য V3Control::addIgnoreMatch(V3ErrorCode code, ধ্রুবক string& filename, ধ্রুবক string& contents,
                               ধ্রুবক string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

শূন্য V3Control::addInline(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& ftask, যৌক্তিক on) {
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

শূন্য V3Control::addModulePragma(ধ্রুবক string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

শূন্য V3Control::add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().add_Profile_তথ্য(fl, hierDpi, cost);
}
শূন্য V3Control::add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& model, ধ্রুবক string& চাবি,
                               uint64_t cost) {
    V3ControlResolver::s().add_Profile_তথ্য(fl, model, চাবি, cost);
}

শূন্য V3Control::addScopeTraceOn(যৌক্তিক on, ধ্রুবক string& scope, পূর্ণসংখ্যা levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

শূন্য V3Control::addVarAttr(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& ftask,
                           VarSpecKind kind, ধ্রুবক string& pattern, VAttrType attr,
                           Ast_Sen_বৃক্ষ* sensep) {
    যদি (sensep) {
        File_সারি* ধ্রুবক flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_বৃক্ষ(), sensep);
        // Used to be only accepted on public_flat_rw
        যদি (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ফেরত;
        }
    }

    যদি (kind != VarSpecKind::VAR) {
        নির্বাচন (attr) {
        ক্ষেত্র VAttrType::VAR_PUBLIC_FLAT:
        ক্ষেত্র VAttrType::VAR_PUBLIC_FLAT_RD:
        ক্ষেত্র VAttrType::VAR_PUBLIC_FLAT_RW: বিরতি;
        পূর্বনির্ধারিত:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ফেরত;
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
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(সত্য);
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
                    যদি (kind == VarSpecKind::PARAM) ফেরত mod.params().at(pattern);
                    যদি (kind == VarSpecKind::PORT) ফেরত mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    ফেরত mod.vars().at(pattern);
                }
                যদি (kind == VarSpecKind::PARAM) ফেরত mod.ftasks().at(ftask).params().at(pattern);
                যদি (kind == VarSpecKind::PORT) ফেরত mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                ফেরত mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

শূন্য V3Control::applyCase(AstCase* nodep) {
    ধ্রুবক string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুবক filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyCase(nodep);
}

শূন্য V3Control::applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstBegin* nodep) {
    ধ্রুবক string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুবক filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyBlock(nodep);
    ধ্রুবক string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুবক modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->applyBlock(nodep);
}
শূন্য V3Control::applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstGenBlock* nodep) {
    ধ্রুবক string& filename = nodep->fileline()->filename();
    V3ControlFile* ধ্রুবক filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyBlock(nodep);
    ধ্রুবক string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুবক modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->applyBlock(nodep);
}

শূন্য V3Control::applyIgnores(File_সারি* filelinep) {
    ধ্রুবক string& filename = filelinep->filename();
    V3ControlFile* ধ্রুবক filep = V3ControlResolver::s().files().resolve(filename);
    যদি (filep) filep->applyIgnores(filelinep);
}

শূন্য V3Control::applyModule(Ast_গ্রন্থি_Module* modulep) {
    ধ্রুবক string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুবক modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (modp) modp->apply(modulep);
}

শূন্য V3Control::applyFTask(Ast_গ্রন্থি_Module* modulep, Ast_গ্রন্থি_FTask* ftaskp) {
    ধ্রুবক string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ধ্রুবক modp = V3ControlResolver::s().modules().resolve(modname);
    যদি (!modp) ফেরত;
    ধ্রুবক V3ControlFTask* ধ্রুবক ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    যদি (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
স্থির শূন্য resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ধ্রুবক std::string name = varp->prettyDehashOrigOrName();
    যদি (ধ্রুবক V3ControlVar* ধ্রুবক vp = resolver.vars().resolve(name)) vp->apply(varp);
    যদি (varp->isParam()) {
        যদি (ধ্রুবক V3ControlVar* ধ্রুবক vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    যদি (varp->isIO()) {
        যদি (ধ্রুবক V3ControlVar* ধ্রুবক vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

শূন্য V3Control::applyVarAttr(ধ্রুবক Ast_গ্রন্থি_Module* modulep, ধ্রুবক Ast_গ্রন্থি_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ধ্রুবক modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    যদি (!modp) ফেরত;
    যদি (ftaskp) {
        V3ControlFTask* ধ্রুবক ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        যদি (!ftp) ফেরত;
        resolveThenApply(*ftp, varp);
    } অন্যথা {
        resolveThenApply(*modp, varp);
    }
}

পূর্ণসংখ্যা V3Control::getHierWorkers(ধ্রুবক string& model) {
    ফেরত V3ControlResolver::s().getHierWorkers(model);
}
File_সারি* V3Control::get_Hier_Workers_File_সারি(ধ্রুবক string& model) {
    ফেরত V3ControlResolver::s().get_Hier_Workers_File_সারি(model);
}
ধ্রুবক V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ধ্রুবক string& module) {
    V3ControlModule* ধ্রুবক modp = V3ControlResolver::s().modules().resolve(module);
    ফেরত modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::get_Profile_তথ্য(ধ্রুবক string& hierDpi) {
    ফেরত V3ControlResolver::s().get_Profile_তথ্য(hierDpi);
}
uint64_t V3Control::get_Profile_তথ্য(ধ্রুবক string& model, ধ্রুবক string& চাবি) {
    ফেরত V3ControlResolver::s().get_Profile_তথ্য(model, চাবি);
}
File_সারি* V3Control::get_Profile_তথ্য_File_সারি() {
    ফেরত V3ControlResolver::s().get_Profile_তথ্য_File_সারি();
}
যৌক্তিক V3Control::getScopeTraceOn(ধ্রুবক string& scope) {
    ফেরত V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

শূন্য V3Control::contents_ঠেলো_Text(ধ্রুবক string& text) { WildcardContents::ঠেলো_Text(text); }

যৌক্তিক V3Control::contains_MTask_Profile_তথ্য() {
    ফেরত V3ControlResolver::s().contains_MTask_Profile_তথ্য();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    ফেরত V3ControlResolver::s().getCurrentHierBlockCost();
}

যৌক্তিক V3Control::waive(ধ্রুবক File_সারি* filelinep, V3ErrorCode code, ধ্রুবক string& message) {
    V3ControlFile* ধ্রুবক filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    যদি (!filep) ফেরত মিথ্যা;
    ফেরত filep->waive(code, message);
}

শূন্য V3Control::selfTest() { VInterval_বৃক্ষ::selfTest(); }
