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

#ضمن "V3PchAstMT.h"

#ضمن "V3File.h"

#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <cerrno>
#ضمن <cstdarg>
#ضمن <fcntl.h>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

// clang-format off
#𐡄𐡍 defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وإلا
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#𐡄𐡍 defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحيح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحيح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

صنف V3FileDependImp final {
    // TYPES
    صنف DependFile final {
        // A single file
        ثابت منطقي m_target;  // True if write, else read
        منطقي m_exists = 𐡒𐡔𐡈;  // File exists
        ثابت string m_filename;  // Filename
        بنية stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عام:
        DependFile(ثابت string& filename, منطقي target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = افتراضي;
        ثابت string& filename() ثابت { أرجع m_filename; }
        منطقي target() ثابت { أرجع m_target; }
        منطقي exists() ثابت { أرجع m_exists; }
        off_t size() ثابت { أرجع m_stat.st_size; }
        ino_t ino() ثابت { أرجع m_stat.st_ino; }
        time_t cstime() ثابت { أرجع m_stat.st_ctime; }  // Seconds
        time_t cnstime() ثابت { أرجع VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ثابت { أرجع m_stat.st_mtime; }  // Seconds
        time_t mnstime() ثابت { أرجع VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hashDigestSymbol() {
            ساكن VHashSha512 s_emptyHash;
            أرجع m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ loadStats() {
            𐡄𐡍 (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                𐡄𐡍 (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = خطأ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وإلا {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    𐡄𐡍 (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.insertFile(filename());
                }
            }
        }
        منطقي operator<(ثابت DependFile& rhs) ثابت { أرجع filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& in) {
        string pretty = in;
        string::size_type pos;
        طالما ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        طالما ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        أرجع pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ addSrcDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.insert(filename);
        𐡄𐡍 (itFoundPair.second) {
            DependFile df{filename, خطأ};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.insert(df);
        }
    }
    فراغ addTgtDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.insert(filename);
        𐡄𐡍 (itFoundPair.second) m_filenameList.insert(DependFile{filename, 𐡒𐡔𐡈});
    }
    فراغ writeDepend(ثابت string& filename);
    std::vector<string> getAllDeps() ثابت;
    فراغ writeTimes(ثابت string& filename, ثابت string& cmdlineIn);
    منطقي checkTimes(ثابت string& filename, ثابت string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

فراغ V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    𐡄𐡍 (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        𐡄𐡍 (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        𐡄𐡍 (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    𐡄𐡍 (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            𐡄𐡍 (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ثابت {
    std::vector<string> r;
    لكل (ثابت auto& itr : m_filenameList) {
        𐡄𐡍 (!itr.target() && itr.exists()) r.push_back(itr.filename());
    }
    أرجع r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    𐡄𐡍 (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    لكل (auto& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        𐡄𐡍 (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hashDigestSymbol() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقي V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    𐡄𐡍 (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        أرجع خطأ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        𐡄𐡍 (ignore.empty()) { /*used*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdlineIn);
        𐡄𐡍 (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            أرجع خطأ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    طالما (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        𐡄𐡍 (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية stat curStat;
        ثابت صحيح err = stat(chkFilename.c_str(), &curStat);
        𐡄𐡍 (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            أرجع خطأ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        𐡄𐡍 (filename == chkFilename) استمر;

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
        𐡄𐡍 (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            𐡄𐡍 (skipHashing) أرجع خطأ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            𐡄𐡍 (chkHash == "unhashed") أرجع خطأ;

            VHashSha512 curHash;
            curHash.insertFile(chkFilename);
            𐡄𐡍 (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                أرجع خطأ;
            }
        }
    }
    أرجع 𐡒𐡔𐡈;
}

//######################################################################
// V3File

فراغ V3File::addSrcDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
فراغ V3File::addTgtDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { أرجع dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    أرجع dependImp.checkTimes(filename, cmdlineIn);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    𐡄𐡍 (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = خطأ;
    𐡄𐡍 (!s_created) {
        s_created = 𐡒𐡔𐡈;
        V3Os::createDir(v3Global.opt.makeDir());
        𐡄𐡍 (v3Global.opt.hierTop()) V3Os::createDir(v3Global.opt.hierTopDataDir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = خطأ;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وإلا
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#endif
    منطقي m_pidExited = خطأ;  //  waitpid() reaped the filter process
    صحيح m_pidStatus = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        𐡄𐡍 (m_pid) {
            أرجع readContentsFilter(filename, outl);
        } وإلا {
            أرجع readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        𐡄𐡍 (fd < 0) أرجع خطأ;
        m_readEof = خطأ;
        readBlocks(fd, -1, outl);
        close(fd);
        أرجع 𐡒𐡔𐡈;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string line = readFilterLine();
        𐡄𐡍 (line.find("Content-Length") != string::npos) {
            صحيح len = 0;
            sscanf(line.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            أرجع 𐡒𐡔𐡈;
        } وإلا {
            𐡄𐡍 (line != "") v3error("--pipe-filter protocol error, unexpected: " << line);
            أرجع خطأ;
        }
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
        أرجع خطأ;
#endif
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#ifdef INFILTER_PIPE
        𐡄𐡍 (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = 𐡒𐡔𐡈;
            m_pidExited = 𐡒𐡔𐡈;
        }
#endif
    }

    فراغ readBlocks(صحيح fd, صحيح size, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        طالما (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            𐡄𐡍 (size > 0 && size < todo) todo = size;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            𐡄𐡍 (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } وإلا 𐡄𐡍 (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                m_readEof = 𐡒𐡔𐡈;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string readFilterLine() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string line;
        طالما (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            line += onechar;
            𐡄𐡍 (onechar == "\n") {
                𐡄𐡍 (line == "\n") {
                    line = "";
                    استمر;
                } وإلا {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << line);
        أرجع line;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& out) {
        𐡄𐡍 (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            𐡄𐡍 (out[out.length() - 1] != '\n') cout << '\n';
        }
        𐡄𐡍 (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = 𐡒𐡔𐡈;
            stop();
        }
        unsigned offset = 0;
        طالما (!m_readEof && out.length() > offset) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            𐡄𐡍 (got > 0) {
                offset += got;
            } وإلا 𐡄𐡍 (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ start(ثابت string& command) {
        𐡄𐡍 (command == "") {
            m_pid = 0;  // Disabled
        } وإلا {
            startFilter(command);
        }
    }
    فراغ startFilter(ثابت string& command) {
        (فراغ)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        صحيح fd_stdin[2];  // Can't use std::array
        صحيح fd_stdout[2];  // Can't use std::array
        constexpr صحيح P_RD = 0;
        constexpr صحيح P_WR = 1;

        𐡄𐡍 (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        𐡄𐡍 (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        𐡄𐡍 (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        𐡄𐡍 (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وإلا {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = خطأ;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = خطأ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    فراغ stop() {
        𐡄𐡍 (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(𐡒𐡔𐡈);
        𐡄𐡍 (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

محمي:
    friend صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت auto it = m_contentsMap.find(filename);
        𐡄𐡍 (it != m_contentsMap.end()) {
            outl.push_back(it->second);
            أرجع 𐡒𐡔𐡈;
        }
        𐡄𐡍 (!readContents(filename, outl)) أرجع خطأ;
        𐡄𐡍 (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        أرجع 𐡒𐡔𐡈;
    }
    ساكن size_t listSize(ثابت StrList& sl) {
        size_t result = 0;
        لكل (ثابت string& i : sl) result += i.length();
        أرجع result;
    }
    ساكن string listString(ثابت StrList& sl) {
        string result;
        لكل (ثابت string& i : sl) result += i;
        أرجع result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ثابت string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جديد VInFilterImp{command}; }
VInFilter::~VInFilter() {
    𐡄𐡍 (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    أرجع m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساكن constexpr صحيح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساكن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    لكل (صحيح i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    أرجع table;
}();

ثابت std::string& V3OutFormatter::indentSpaces(صحيح num) {
    // Indent the specified number of spaces.
    أرجع s_indentSpaces[std::max(0, std::min(num, MAXSPACE))];
}

منطقي V3OutFormatter::tokenMatch(ثابت حرف* cp, ثابت حرف* cmp) {
    طالما (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    𐡄𐡍 (*cmp) أرجع خطأ;
    𐡄𐡍 (*cp && !std::isspace(*cp)) أرجع خطأ;
    أرجع 𐡒𐡔𐡈;
}

منطقي V3OutFormatter::tokenStart(ثابت حرف* cp) {
    أرجع (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

منطقي V3OutFormatter::tokenEnd(ثابت حرف* cp) {
    أرجع (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

منطقي V3OutFormatter::tokenNotStart(ثابت حرف* cp) {
    أرجع (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

صحيح V3OutFormatter::endLevels(ثابت حرف* strg) {
    صحيح levels = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        طالما (std::isspace(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            أرجع 0;
        حالة '#':  // Preproc directive
            𐡄𐡍 (m_lang == LA_C) أرجع 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::isalnum(*mp); ++mp) {}
            𐡄𐡍 (mp[0] == ':' && mp[1] != ':') أرجع (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': levels -= m_blockIndent; اكسر;
        حالة '<':
            𐡄𐡍 (m_lang == LA_XML) {
                𐡄𐡍 (cp[1] == '/') levels -= m_blockIndent;
            }
            اكسر;
        حالة 'e':
            𐡄𐡍 (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: أرجع levels;  // Letter
        }
    }
    أرجع levels;
}

فراغ V3OutFormatter::putns(ثابت AstNode* nodep, ثابت حرف* strg) {
    𐡄𐡍 (!v3Global.opt.decoration()) {
        putsOutput(strg);
        أرجع;
    }

    ثابت منطقي putNodeDecoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && FileLine::builtInFilename() != nodep->fileline()->filename();

    𐡄𐡍 (m_prependIndent && ((strg[0] && strg[0] != '\n') || putNodeDecoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = خطأ;
    }

    𐡄𐡍 (putNodeDecoration) {
        ثابت FileLine* ثابت flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ثابت std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    منطقي notstart = خطأ;
    منطقي wordstart = 𐡒𐡔𐡈;
    منطقي equalsForBracket = خطأ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        𐡄𐡍 (std::isalpha(*cp)) {
            𐡄𐡍 (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = 𐡒𐡔𐡈;
            𐡄𐡍 (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            𐡄𐡍 (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ثابت منطقي wasInBackslash = m_inBackslash;
        m_inBackslash = خطأ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = 𐡒𐡔𐡈;
            𐡄𐡍 (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = 𐡒𐡔𐡈;
            } وإلا {
                m_prependIndent = خطأ;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = 𐡒𐡔𐡈; اكسر;
        حالة '\t': wordstart = 𐡒𐡔𐡈; اكسر;
        حالة '"':
            wordstart = خطأ;
            𐡄𐡍 (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            اكسر;
        حالة '\\':
            wordstart = خطأ;
            𐡄𐡍 (!wasInBackslash) m_inBackslash = 𐡒𐡔𐡈;
            اكسر;
        حالة '/':
            𐡄𐡍 (m_lang == LA_C || m_lang == LA_VERILOG) {
                𐡄𐡍 (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    طالما (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    𐡄𐡍 (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            𐡄𐡍 (!m_inStringLiteral) {
                𐡄𐡍 (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.push(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            𐡄𐡍 (!m_inStringLiteral) {
                𐡄𐡍 (m_bracketLevel > 0) {
                    m_parenVec.pop();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            𐡄𐡍 (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.push(m_column);
            }
            اكسر;
        حالة ')':
            𐡄𐡍 (!m_inStringLiteral) {
                𐡄𐡍 (!m_parenVec.empty()) m_parenVec.pop();
                indentDec();
            }
            اكسر;
        حالة '<':
            𐡄𐡍 (m_lang == LA_XML) {
                𐡄𐡍 (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وإلا 𐡄𐡍 (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وإلا {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            𐡄𐡍 (m_lang == LA_XML) {
                indentDec();
                𐡄𐡍 (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = خطأ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = 𐡒𐡔𐡈; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = خطأ; اكسر;
        }
    }
}

فراغ V3OutFormatter::putBreakExpr() {
    𐡄𐡍 (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
فراغ V3OutFormatter::putBreak() {
    𐡄𐡍 (!v3Global.opt.decoration()) أرجع;
    𐡄𐡍 (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        𐡄𐡍 (exceededWidth()) {
            putcNoTracking('\n');
            𐡄𐡍 (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.top()));
        }
    }
}

فراغ V3OutFormatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    لكل (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    𐡄𐡍 (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3OutFormatter::putsNoTracking(ثابت string& strg) {
    𐡄𐡍 (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        أرجع;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3OutFormatter::putcNoTracking(حرف chr) {
    𐡄𐡍 (!v3Global.opt.decoration()) {
        putcOutput(chr);
        أرجع;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = 𐡒𐡔𐡈;
        اكسر;
    حالة '\t': m_column = ((m_column + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_column; اكسر;
    افتراضي:
        ++m_column;
        m_nobreak = خطأ;
        اكسر;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ثابت string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    𐡄𐡍 (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            𐡄𐡍 (c == '"') {
                out += "&quot;"s;
            } وإلا 𐡄𐡍 (c == '\'') {
                out += "&apos;"s;
            } وإلا 𐡄𐡍 (c == '<') {
                out += "&lt;"s;
            } وإلا 𐡄𐡍 (c == '>') {
                out += "&gt;"s;
            } وإلا 𐡄𐡍 (c == '&') {
                out += "&amp;"s;
            } وإلا 𐡄𐡍 (std::isprint(c)) {
                out += c;
            } وإلا {
                out += "&#"s + cvtToStr(static_cast<unsigned صحيح>(c & 0xff)) + ";";
            }
        }
    } وإلا {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            𐡄𐡍 (c == '\\' || c == '"') {
                out += "\\"s + c;
            } وإلا 𐡄𐡍 (c == '\n') {
                out += "\\n";
            } وإلا 𐡄𐡍 (c == '\r') {
                out += "\\r";
            } وإلا 𐡄𐡍 (c == '\t') {
                out += "\\t";
            } وإلا 𐡄𐡍 (std::isprint(c)) {
                out += c;
            } وإلا {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    أرجع out;
}

//----------------------------------------------------------------------
// Simple wrappers

فراغ V3OutFormatter::printf(ثابت حرف* fmt...) {
    constexpr size_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    هذا->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ثابت string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{جديد std::array<حرف, WRITE_BUFFER_SIZE_BYTES>{}} {
    𐡄𐡍 ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    𐡄𐡍 (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

فراغ V3OutFile::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = 𐡒𐡔𐡈;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        𐡄𐡍 (!std::isalnum(c)) c = '_';
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

صنف VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
محمي:
    // CONSTRUCTORS
    friend صنف VIdProtect;
    ساكن VIdProtectImp& singleton() VL_MT_SAFE {
        ساكن VIdProtectImp s;
        أرجع s;
    }

عام:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = افتراضي;
    // METHODS
    string passthru(ثابت string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        𐡄𐡍 (!v3Global.opt.protectIds()) أرجع old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto it = m_nameMap.find(old);
        𐡄𐡍 (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وإلا {
            m_nameMap.emplace(old, old);
            m_newIdSet.insert(old);
        }
        أرجع old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        𐡄𐡍 (!v3Global.opt.protectIds() || old.empty() || !doIt) أرجع old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto pair = m_nameMap.emplace(old, "");
        𐡄𐡍 (pair.second) {
            string out;
            𐡄𐡍 (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } وإلا {
                VHashSha512 digest{v3Global.opt.protectKeyDefaulted()};
                digest.insert(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (size_t len = 6; len < out.size() - 3; len += 3) {
                    ثابت string tryout = out.substr(0, len);
                    𐡄𐡍 (m_newIdSet.insert(tryout).second) {
                        out = tryout;
                        اكسر;
                    }
                }
            }
            pair.first->second = out;
        }
        أرجع pair.first->second;
    }
    string protectWordsIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        𐡄𐡍 (!(doIt && v3Global.opt.protectIds())) أرجع old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        طالما (𐡒𐡔𐡈) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            𐡄𐡍 (pos == string::npos) اكسر;
            out += protectIf(old.substr(start, pos - start), 𐡒𐡔𐡈) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), 𐡒𐡔𐡈);
        أرجع out;
    }
    فراغ writeMapFile(ثابت string& filename) ثابت {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            لكل (ثابت auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

خاص:
    فراغ trySep(ثابت string& old, string::size_type start, ثابت string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ثابت string::size_type trypos = old.find(trySep, start);
        𐡄𐡍 (trypos != string::npos) {
            𐡄𐡍 (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& in) VL_MT_SAFE {
    أرجع v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
    أرجع VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protectWordsIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
    أرجع VIdProtectImp::singleton().protectWordsIf(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
