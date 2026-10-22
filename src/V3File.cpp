// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <cerrno>
#ଅନ୍ତର୍ଭାଵ <cstdarg>
#ଅନ୍ତର୍ଭାଵ <fcntl.h>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>

#ଅନ୍ତର୍ଭାଵ <sys/stat.h>
#ଅନ୍ତର୍ଭାଵ <sys/types.h>

// clang-format off
#ଯଦି defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ଅନ୍ୟଥା
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#ଯଦି defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

ଶ୍ରେଣୀ V3FileDependImp final {
    // TYPES
    ଶ୍ରେଣୀ DependFile final {
        // A single file
        ଅଚଳ ତାର୍କିକ m_target;  // True if write, else read
        ତାର୍କିକ m_exists = ସତ୍ୟ;  // File exists
        ଅଚଳ string m_filename;  // Filename
        ସଂରଚନା stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    ସର୍ବସାଧାରଣ:
        DependFile(ଅଚଳ string& filename, ତାର୍କିକ target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ଅଚଳ string& filename() ଅଚଳ { ଫେରନ୍ତୁ m_filename; }
        ତାର୍କିକ target() ଅଚଳ { ଫେରନ୍ତୁ m_target; }
        ତାର୍କିକ exists() ଅଚଳ { ଫେରନ୍ତୁ m_exists; }
        off_t size() ଅଚଳ { ଫେରନ୍ତୁ m_stat.st_size; }
        ino_t ino() ଅଚଳ { ଫେରନ୍ତୁ m_stat.st_ino; }
        time_t cstime() ଅଚଳ { ଫେରନ୍ତୁ m_stat.st_ctime; }  // Seconds
        time_t cnstime() ଅଚଳ { ଫେରନ୍ତୁ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ଅଚଳ { ଫେରନ୍ତୁ m_stat.st_mtime; }  // Seconds
        time_t mnstime() ଅଚଳ { ଫେରନ୍ତୁ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_ପ୍ରତୀକ() {
            ସ୍ଥିର VHashSha512 s_emptyHash;
            ଫେରନ୍ତୁ m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        ଶୂନ୍ୟ loadStats() {
            ଯଦି (!m_stat.st_mtime) {
                ଅଚଳ string fn = filename();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା err = stat(fn.c_str(), &m_stat);
                ଯଦି (err != 0) {
                    std::memset(&m_stat, 0, ଆକାର(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ମିଥ୍ୟା;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ଅନ୍ୟଥା {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ଯଦି (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.ଭର୍ତ୍ତି_File(filename());
                }
            }
        }
        ତାର୍କିକ operator<(ଅଚଳ DependFile& rhs) ଅଚଳ { ଫେରନ୍ତୁ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    ସ୍ଥିର string stripQuotes(ଅଚଳ string& in) {
        string pretty = in;
        string::size_type pos;
        ଯେତେବେଳେ ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ଯେତେବେଳେ ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ଫେରନ୍ତୁ pretty;
    }

ସର୍ବସାଧାରଣ:
    // ACCESSOR METHODS
    ଶୂନ୍ୟ addSrcDepend(ଅଚଳ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଅଚଳ auto itFoundPair = m_filenameSet.ଭର୍ତ୍ତି(filename);
        ଯଦି (itFoundPair.second) {
            DependFile df{filename, ମିଥ୍ୟା};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.ଭର୍ତ୍ତି(df);
        }
    }
    ଶୂନ୍ୟ addTgtDepend(ଅଚଳ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଅଚଳ auto itFoundPair = m_filenameSet.ଭର୍ତ୍ତି(filename);
        ଯଦି (itFoundPair.second) m_filenameList.ଭର୍ତ୍ତି(DependFile{filename, ସତ୍ୟ});
    }
    ଶୂନ୍ୟ writeDepend(ଅଚଳ string& filename);
    std::vector<string> getAllDeps() ଅଚଳ;
    ଶୂନ୍ୟ writeTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn);
    ତାର୍କିକ checkTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

ଶୂନ୍ୟ V3FileDependImp::writeDepend(ଅଚଳ string& filename) {
    ଅଚଳ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ଯଦି (ofp->fail()) v3fatal("Can't write file: " << filename);

    ପାଇଁ (ଅଚଳ DependFile& i : m_filenameList) {
        ଯଦି (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    ପାଇଁ (ଅଚଳ DependFile& i : m_filenameList) {
        ଯଦି (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ଯଦି (v3Global.opt.makePhony()) {
        *ofp << '\n';
        ପାଇଁ (ଅଚଳ DependFile& i : m_filenameList) {
            ଯଦି (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ଅଚଳ {
    std::vector<string> r;
    ପାଇଁ (ଅଚଳ auto& itr : m_filenameList) {
        ଯଦି (!itr.target() && itr.exists()) r.ଠେଲ_back(itr.filename());
    }
    ଫେରନ୍ତୁ r;
}

ଶୂନ୍ୟ V3FileDependImp::writeTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn) {
    ଅଚଳ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ଯଦି (ofp->fail()) v3fatal("Can't write file: " << filename);

    ଅଚଳ string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    ପାଇଁ (auto& it : m_filenameList) {
        DependFile* ଅଚଳ depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        ଯଦି (depfilep->filename() == filename) {
            showSize = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << showSize;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_ପ୍ରତୀକ() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

ତାର୍କିକ V3FileDependImp::checkTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn) {
    ଅଚଳ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    ଯଦି (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    {
        ଅଚଳ string ignore = V3Os::getline(*ifp);
        ଯଦି (ignore.empty()) { /*used*/
        }
    }
    {
        ଅକ୍ଷର chkDir;
        *ifp >> chkDir;
        ଅକ୍ଷର quote;
        *ifp >> quote;
        ଅଚଳ string chkCmdline = V3Os::getline(*ifp, '"');
        ଅଚଳ string cmdline = stripQuotes(cmdlineIn);
        ଯଦି (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
    }

    ଅଚଳ ତାର୍କିକ skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    ଯେତେବେଳେ (!ifp->eof()) {
        ଅକ୍ଷର chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        ଯଦି (ifp->eof()) ଭାଙ୍ଗ;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        ଅକ୍ଷର quote;
        *ifp >> quote;
        ଅଚଳ string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ଅଚଳ string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ସଂରଚନା stat curStat;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା err = stat(chkFilename.c_str(), &curStat);
        ଯଦି (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ଯଦି (filename == chkFilename) ଜାରି;

        UINFO(9, " got d=" << chkDir << " s=" << chkSize << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_size << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        ଯଦି (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ଯଦି (skipHashing) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ଯଦି (chkHash == "unhashed") ଫେରନ୍ତୁ ମିଥ୍ୟା;

            VHashSha512 curHash;
            curHash.ଭର୍ତ୍ତି_File(chkFilename);
            ଯଦି (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

//######################################################################
// V3File

ଶୂନ୍ୟ V3File::addSrcDepend(ଅଚଳ string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
ଶୂନ୍ୟ V3File::addTgtDepend(ଅଚଳ string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
ଶୂନ୍ୟ V3File::writeDepend(ଅଚଳ string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { ଫେରନ୍ତୁ dependImp.getAllDeps(); }
ଶୂନ୍ୟ V3File::writeTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
ତାର୍କିକ V3File::checkTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn) {
    ଫେରନ୍ତୁ dependImp.checkTimes(filename, cmdlineIn);
}
ଶୂନ୍ୟ V3File::createMakeDirFor(ଅଚଳ string& filename) {
    ଯଦି (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
ଶୂନ୍ୟ V3File::createMakeDir() {
    ସ୍ଥିର ତାର୍କିକ s_created = ମିଥ୍ୟା;
    ଯଦି (!s_created) {
        s_created = ସତ୍ୟ;
        V3Os::createDir(v3Global.opt.makeDir());
        ଯଦି (v3Global.opt.hier_ଶୀର୍ଷ()) V3Os::createDir(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

ଶ୍ରେଣୀ VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ଅଚଳ std::string, std::string> m_contentsMap;  // Cache of file contents
    ତାର୍କିକ m_readEof = ମିଥ୍ୟା;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ଅନ୍ୟଥା
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pid = 0;  // fork() process id - always zero as disabled
#endif
    ତାର୍କିକ m_pidExited = ମିଥ୍ୟା;  //  waitpid() reaped the filter process
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pidStatus = 0;  // Exit status of the filter process
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_writeFd = 0;  // File descriptor TO filter
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_readFd = 0;  // File descriptor FROM filter

ବ୍ୟକ୍ତିଗତ:
    // METHODS

    ତାର୍କିକ readContents(ଅଚଳ string& filename, StrList& outl) {
        ଯଦି (m_pid) {
            ଫେରନ୍ତୁ readContentsFilter(filename, outl);
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ readContentsFile(filename, outl);
        }
    }
    ତାର୍କିକ readContentsFile(ଅଚଳ string& filename, StrList& outl) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = open(filename.c_str(), O_RDONLY);
        ଯଦି (fd < 0) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        m_readEof = ମିଥ୍ୟା;
        readBlocks(fd, -1, outl);
        close(fd);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    // cppcheck-suppress constParameterReference
    ତାର୍କିକ readContentsFilter(ଅଚଳ string& filename, StrList& outl) {
        (ଶୂନ୍ୟ)filename;  // Prevent unused variable warning
        (ଶୂନ୍ୟ)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ଅଚଳ string ଧାଡ଼ି = read_Filter_ଧାଡ଼ି();
        ଯଦି (ଧାଡ଼ି.find("Content-Length") != string::npos) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା len = 0;
            sscanf(ଧାଡ଼ି.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            ଯଦି (ଧାଡ଼ି != "") v3error("--pipe-filter protocol error, unexpected: " << ଧାଡ଼ି);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
#ଅନ୍ୟଥା
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
    }

    // cppcheck-suppress functionConst
    ଶୂନ୍ୟ checkFilter(ତାର୍କିକ hang) {
#ifdef INFILTER_PIPE
        ଯଦି (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = ସତ୍ୟ;
            m_pidExited = ସତ୍ୟ;
        }
#endif
    }

    ଶୂନ୍ୟ readBlocks(ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd, ପୂର୍ଣ୍ଣସଂଖ୍ୟା size, StrList& outl) {
        ଅକ୍ଷର buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ଯେତେବେଳେ (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ଯଦି (size > 0 && size < todo) todo = size;
            errno = 0;
            ଅଚଳ ssize_t got = ପଠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ଯଦି (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } ଅନ୍ୟଥା ଯଦି (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ମିଥ୍ୟା);
                V3Os::u_sleep(1000);
                ଜାରି;
            } ଅନ୍ୟଥା {
                m_readEof = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_ଧାଡ଼ି() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string ଧାଡ଼ି;
        ଯେତେବେଳେ (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ଅଚଳ string onechar = listString(outl);
            ଧାଡ଼ି += onechar;
            ଯଦି (onechar == "\n") {
                ଯଦି (ଧାଡ଼ି == "\n") {
                    ଧାଡ଼ି = "";
                    ଜାରି;
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
                }
            }
        }
        UINFO(6, "filter-line-in: " << ଧାଡ଼ି);
        ଫେରନ୍ତୁ ଧାଡ଼ି;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    ଶୂନ୍ୟ writeFilter(ଅଚଳ string& out) {
        ଯଦି (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            ଯଦି (out[out.length() - 1] != '\n') cout << '\n';
        }
        ଯଦି (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ସତ୍ୟ;
            stop();
        }
        unsigned offset = 0;
        ଯେତେବେଳେ (!m_readEof && out.length() > offset) {
            errno = 0;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା got = ଲିଖ(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ଯଦି (got > 0) {
                offset += got;
            } ଅନ୍ୟଥା ଯଦି (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ମିଥ୍ୟା);
                V3Os::u_sleep(1000);
                ଜାରି;
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        }
    }

    // Start the filter
    ଶୂନ୍ୟ start(ଅଚଳ string& command) {
        ଯଦି (command == "") {
            m_pid = 0;  // Disabled
        } ଅନ୍ୟଥା {
            startFilter(command);
        }
    }
    ଶୂନ୍ୟ startFilter(ଅଚଳ string& command) {
        (ଶୂନ୍ୟ)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd_stdin[2];  // Can't use std::array
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd_stdout[2];  // Can't use std::array
        constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା P_RD = 0;
        constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା P_WR = 1;

        ଯଦି (VL_UNCOVERABLE(ପ୍ରଣାଲ(fd_stdin) != 0 || ପ୍ରଣାଲ(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ଯଦି (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ଅଚଳ pid_t pid = fork();
        ଯଦି (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ଯଦି (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<ଅକ୍ଷର*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } ଅନ୍ୟଥା {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ମିଥ୍ୟା;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ମିଥ୍ୟା;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            ପୂର୍ଣ୍ଣସଂଖ୍ୟା flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ଅନ୍ୟଥା
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    ଶୂନ୍ୟ stop() {
        ଯଦି (m_pid) stopFilter();
    }
    ଶୂନ୍ୟ stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ସତ୍ୟ);
        ଯଦି (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ଅନ୍ୟଥା
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

ସୁରକ୍ଷିତ:
    friend ଶ୍ରେଣୀ VInFilter;
    // Read file contents and return it
    ତାର୍କିକ readWholefile(ଅଚଳ string& filename, StrList& outl) {
        ଅଚଳ auto it = m_contentsMap.find(filename);
        ଯଦି (it != m_contentsMap.end()) {
            outl.ଠେଲ_back(it->second);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (!readContents(filename, outl)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ସ୍ଥିର size_t listSize(ଅଚଳ StrList& sl) {
        size_t result = 0;
        ପାଇଁ (ଅଚଳ string& i : sl) result += i.length();
        ଫେରନ୍ତୁ result;
    }
    ସ୍ଥିର string listString(ଅଚଳ StrList& sl) {
        string result;
        ପାଇଁ (ଅଚଳ string& i : sl) result += i;
        ଫେରନ୍ତୁ result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ଅଚଳ string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ଅଚଳ string& command) { m_impp = ନୂତନ VInFilterImp{command}; }
VInFilter::~VInFilter() {
    ଯଦି (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

ତାର୍କିକ VInFilter::readWholefile(ଅଚଳ string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ଫେରନ୍ତୁ m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ସ୍ଥିର ଅଚଳ std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    ଫେରନ୍ତୁ table;
}();

ଅଚଳ std::string& V3OutFormatter::indentSpaces(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ) {
    // Indent the specified number of spaces.
    ଫେରନ୍ତୁ s_indentSpaces[std::max(0, std::min(ଅଙ୍କ, MAXSPACE))];
}

ତାର୍କିକ V3OutFormatter::tokenMatch(ଅଚଳ ଅକ୍ଷର* cp, ଅଚଳ ଅକ୍ଷର* cmp) {
    ଯେତେବେଳେ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ଯଦି (*cmp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (*cp && !std::isspace(*cp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ତାର୍କିକ V3OutFormatter::tokenStart(ଅଚଳ ଅକ୍ଷର* cp) {
    ଫେରନ୍ତୁ (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

ତାର୍କିକ V3OutFormatter::tokenEnd(ଅଚଳ ଅକ୍ଷର* cp) {
    ଫେରନ୍ତୁ (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

ତାର୍କିକ V3OutFormatter::tokenNotStart(ଅଚଳ ଅକ୍ଷର* cp) {
    ଫେରନ୍ତୁ (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3OutFormatter::endLevels(ଅଚଳ ଅକ୍ଷର* strg) {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels = m_indentLevel;
    {
        ଅଚଳ ଅକ୍ଷର* cp = strg;
        ଯେତେବେଳେ (std::isspace(*cp)) ++cp;
        ବାଛ (*cp) {
        ସ୍ଥିତି '\n':  // Newlines.. No need for whitespace before it
            ଫେରନ୍ତୁ 0;
        ସ୍ଥିତି '#':  // Preproc directive
            ଯଦି (m_lang == LA_C) ଫେରନ୍ତୁ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ଅଚଳ ଅକ୍ଷର* mp = cp;
            ପାଇଁ (; std::isalnum(*mp); ++mp) {}
            ଯଦି (mp[0] == ':' && mp[1] != ':') ଫେରନ୍ତୁ (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = strg; *cp; ++cp) {
        ବାଛ (*cp) {
        ସ୍ଥିତି '}':
        ସ୍ଥିତି ')': levels -= m_blockIndent; ଭାଙ୍ଗ;
        ସ୍ଥିତି '<':
            ଯଦି (m_lang == LA_XML) {
                ଯଦି (cp[1] == '/') levels -= m_blockIndent;
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି 'e':
            ଯଦି (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '\t':
        ସ୍ଥିତି ' ': ଭାଙ୍ଗ;  // Continue
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ levels;  // Letter
        }
    }
    ଫେରନ୍ତୁ levels;
}

ଶୂନ୍ୟ V3OutFormatter::putns(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* strg) {
    ଯଦି (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ ତାର୍କିକ put_ଗଣ୍ଠି_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_ଧାଡ଼ି::builtInFilename() != nodep->fileline()->filename();

    ଯଦି (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ଗଣ୍ଠି_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = ମିଥ୍ୟା;
    }

    ଯଦି (put_ଗଣ୍ଠି_Decoration) {
        ଅଚଳ File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ଅଚଳ std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    ତାର୍କିକ notstart = ମିଥ୍ୟା;
    ତାର୍କିକ wordstart = ସତ୍ୟ;
    ତାର୍କିକ equalsForBracket = ମିଥ୍ୟା;  // Looking for "= {"
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ଯଦି (std::isalpha(*cp)) {
            ଯଦି (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = ସତ୍ୟ;
            ଯଦି (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            ଯଦି (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ଅଚଳ ତାର୍କିକ wasInBackslash = m_inBackslash;
        m_inBackslash = ମିଥ୍ୟା;
        ବାଛ (*cp) {
        ସ୍ଥିତି '\n':
            ++m_lineno;
            wordstart = ସତ୍ୟ;
            ଯଦି (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                m_prependIndent = ମିଥ୍ୟା;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି ' ': wordstart = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି '\t': wordstart = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି '"':
            wordstart = ମିଥ୍ୟା;
            ଯଦି (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '\\':
            wordstart = ମିଥ୍ୟା;
            ଯଦି (!wasInBackslash) m_inBackslash = ସତ୍ୟ;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '/':
            ଯଦି (m_lang == LA_C || m_lang == LA_VERILOG) {
                ଯଦି (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ଯେତେବେଳେ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ଯଦି (*cp) putcNoTracking(*cp);
                }
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '{':
            ଯଦି (!m_inStringLiteral) {
                ଯଦି (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ଠେଲ(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '}':
            ଯଦି (!m_inStringLiteral) {
                ଯଦି (m_bracketLevel > 0) {
                    m_parenVec.କାଢ଼();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '(':
            ଯଦି (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ଠେଲ(m_column);
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି ')':
            ଯଦି (!m_inStringLiteral) {
                ଯଦି (!m_parenVec.empty()) m_parenVec.କାଢ଼();
                indentDec();
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '<':
            ଯଦି (m_lang == LA_XML) {
                ଯଦି (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ଅନ୍ୟଥା ଯଦି (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ଅନ୍ୟଥା {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '>':
            ଯଦି (m_lang == LA_XML) {
                indentDec();
                ଯଦି (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: wordstart = ମିଥ୍ୟା; ଭାଙ୍ଗ;
        }

        ବାଛ (*cp) {
        ସ୍ଥିତି '=': equalsForBracket = ସତ୍ୟ; ଭାଙ୍ଗ;
        ସ୍ଥିତି ' ': ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: equalsForBracket = ମିଥ୍ୟା; ଭାଙ୍ଗ;
        }
    }
}

ଶୂନ୍ୟ V3OutFormatter::put_Break_ପଦସମଷ୍ଟି() {
    ଯଦି (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
ଶୂନ୍ୟ V3OutFormatter::putBreak() {
    ଯଦି (!v3Global.opt.decoration()) ଫେରନ୍ତୁ;
    ଯଦି (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ଯଦି (exceededWidth()) {
            putcNoTracking('\n');
            ଯଦି (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.ଶୀର୍ଷ()));
        }
    }
}

ଶୂନ୍ୟ V3OutFormatter::putsQuoted(ଅଚଳ string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ଅଚଳ string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    ଯଦି (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
ଶୂନ୍ୟ V3OutFormatter::putsNoTracking(ଅଚଳ string& strg) {
    ଯଦି (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ଫେରନ୍ତୁ;
    }
    // Don't track {}'s, probably because it's a $display format string
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : strg) putcNoTracking(c);
}

ଶୂନ୍ୟ V3OutFormatter::putcNoTracking(ଅକ୍ଷର chr) {
    ଯଦି (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ଫେରନ୍ତୁ;
    }
    ବାଛ (chr) {
    ସ୍ଥିତି '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = ସତ୍ୟ;
        ଭାଙ୍ଗ;
    ସ୍ଥିତି '\t': m_column = ((m_column + 9) / 8) * 8; ଭାଙ୍ଗ;
    ସ୍ଥିତି ' ':
    ସ୍ଥିତି '(':
    ସ୍ଥିତି '|':
    ସ୍ଥିତି '&': ++m_column; ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
        ++m_column;
        m_nobreak = ମିଥ୍ୟା;
        ଭାଙ୍ଗ;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ଅଚଳ string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    ଯଦି (lang == LA_XML) {
        // Encode chars into XML string
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : namein) {
            ଯଦି (c == '"') {
                out += "&quot;"s;
            } ଅନ୍ୟଥା ଯଦି (c == '\'') {
                out += "&apos;"s;
            } ଅନ୍ୟଥା ଯଦି (c == '<') {
                out += "&lt;"s;
            } ଅନ୍ୟଥା ଯଦି (c == '>') {
                out += "&gt;"s;
            } ଅନ୍ୟଥା ଯଦି (c == '&') {
                out += "&amp;"s;
            } ଅନ୍ୟଥା ଯଦି (std::isprint(c)) {
                out += c;
            } ଅନ୍ୟଥା {
                out += "&#"s + cvtToStr(static_cast<unsigned ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(c & 0xff)) + ";";
            }
        }
    } ଅନ୍ୟଥା {
        // Encode control chars into C style escapes
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : namein) {
            ଯଦି (c == '\\' || c == '"') {
                out += "\\"s + c;
            } ଅନ୍ୟଥା ଯଦି (c == '\n') {
                out += "\\n";
            } ଅନ୍ୟଥା ଯଦି (c == '\r') {
                out += "\\r";
            } ଅନ୍ୟଥା ଯଦି (c == '\t') {
                out += "\\t";
            } ଅନ୍ୟଥା ଯଦି (std::isprint(c)) {
                out += c;
            } ଅନ୍ୟଥା {
                // This will also cover \a etc
                ଅଚଳ string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    ଫେରନ୍ତୁ out;
}

//----------------------------------------------------------------------
// Simple wrappers

ଶୂନ୍ୟ V3OutFormatter::printf(ଅଚଳ ଅକ୍ଷର* fmt...) {
    constexpr size_t bufsize = 5000;
    ଅକ୍ଷର sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    ଏହି->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ଅଚଳ string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{ନୂତନ std::array<ଅକ୍ଷର, WRITE_BUFFER_SIZE_BYTES>{}} {
    ଯଦି ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    ଯଦି (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

ଶୂନ୍ୟ V3OutFile::putsForceIncs() {
    ଅଚଳ VStringList& forceIncs = v3Global.opt.forceIncs();
    ପାଇଁ (ଅଚଳ string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

ଶୂନ୍ୟ V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ସତ୍ୟ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    ପାଇଁ (ଅକ୍ଷର& c : var) {
        ଯଦି (!std::isalnum(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

ଶ୍ରେଣୀ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ଅଚଳ std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    friend ଶ୍ରେଣୀ VIdProtect;
    ସ୍ଥିର VIdProtectImp& singleton() VL_MT_SAFE {
        ସ୍ଥିର VIdProtectImp s;
        ଫେରନ୍ତୁ s;
    }

ସର୍ବସାଧାରଣ:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    string passthru(ଅଚଳ string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଯଦି (!v3Global.opt.protectIds()) ଫେରନ୍ତୁ old;
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଅଚଳ auto it = m_nameMap.find(old);
        ଯଦି (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ଅନ୍ୟଥା {
            m_nameMap.emplace(old, old);
            m_newIdSet.ଭର୍ତ୍ତି(old);
        }
        ଫେରନ୍ତୁ old;
    }
    string protectIf(ଅଚଳ string& old, ତାର୍କିକ doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଯଦି (!v3Global.opt.protectIds() || old.empty() || !doIt) ଫେରନ୍ତୁ old;
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଅଚଳ auto pair = m_nameMap.emplace(old, "");
        ଯଦି (pair.second) {
            string out;
            ଯଦି (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } ଅନ୍ୟଥା {
                VHashSha512 digest{v3Global.opt.protect_ଚାବି_Defaulted()};
                digest.ଭର୍ତ୍ତି(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                ପାଇଁ (size_t len = 6; len < out.size() - 3; len += 3) {
                    ଅଚଳ string tryout = out.substr(0, len);
                    ଯଦି (m_newIdSet.ଭର୍ତ୍ତି(tryout).second) {
                        out = tryout;
                        ଭାଙ୍ଗ;
                    }
                }
            }
            pair.first->second = out;
        }
        ଫେରନ୍ତୁ pair.first->second;
    }
    string protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ଅଚଳ string& old, ତାର୍କିକ doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ଯଦି (!(doIt && v3Global.opt.protectIds())) ଫେରନ୍ତୁ old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        ଯେତେବେଳେ (ସତ୍ୟ) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            ଯଦି (pos == string::npos) ଭାଙ୍ଗ;
            out += protectIf(old.substr(start, pos - start), ସତ୍ୟ) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), ସତ୍ୟ);
        ଫେରନ୍ତୁ out;
    }
    ଶୂନ୍ୟ writeMapFile(ଅଚଳ string& filename) ଅଚଳ {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            ପାଇଁ (ଅଚଳ auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ trySep(ଅଚଳ string& old, string::size_type start, ଅଚଳ string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ଅଚଳ string::size_type trypos = old.find(trySep, start);
        ଯଦି (trypos != string::npos) {
            ଯଦି (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ଅଚଳ string& in) VL_MT_SAFE {
    ଫେରନ୍ତୁ v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ଅଚଳ string& old, ତାର୍କିକ doIt) VL_MT_SAFE {
    ଫେରନ୍ତୁ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ଅଚଳ string& old, ତାର୍କିକ doIt) VL_MT_SAFE {
    ଫେରନ୍ତୁ VIdProtectImp::singleton().protect_ଶବ୍ଦଗୁଡ଼ିକ_If(old, doIt);
}
ଶୂନ୍ୟ VIdProtect::writeMapFile(ଅଚଳ string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
