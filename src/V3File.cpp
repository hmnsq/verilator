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
#ܐܢ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ܐܚܪܢܐ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#ܐܢ defined(_WIN32) || defined(__MINGW32__)
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
        منطقي m_exists = ܫܪܪܐ;  // File exists
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
        ثابت string& filename() ثابت { ܦܢܐ m_filename; }
        منطقي target() ثابت { ܦܢܐ m_target; }
        منطقي exists() ثابت { ܦܢܐ m_exists; }
        off_t size() ثابت { ܦܢܐ m_stat.st_size; }
        ino_t ino() ثابت { ܦܢܐ m_stat.st_ino; }
        time_t cstime() ثابت { ܦܢܐ m_stat.st_ctime; }  // Seconds
        time_t cnstime() ثابت { ܦܢܐ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ثابت { ܦܢܐ m_stat.st_mtime; }  // Seconds
        time_t mnstime() ثابت { ܦܢܐ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hashDigestSymbol() {
            ساكن VHashSha512 s_emptyHash;
            ܦܢܐ m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ loadStats() {
            ܐܢ (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                ܐܢ (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ܕܓܠܐ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ܐܚܪܢܐ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ܐܢ (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.insertFile(filename());
                }
            }
        }
        منطقي operator<(ثابت DependFile& rhs) ثابت { ܦܢܐ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& in) {
        string pretty = in;
        string::size_type pos;
        ܟܡܐ ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ܟܡܐ ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ܦܢܐ pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ addSrcDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.insert(filename);
        ܐܢ (itFoundPair.second) {
            DependFile df{filename, ܕܓܠܐ};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.insert(df);
        }
    }
    فراغ addTgtDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.insert(filename);
        ܐܢ (itFoundPair.second) m_filenameList.insert(DependFile{filename, ܫܪܪܐ});
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
    ܐܢ (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        ܐܢ (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        ܐܢ (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ܐܢ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            ܐܢ (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ثابت {
    std::vector<string> r;
    لكل (ثابت auto& itr : m_filenameList) {
        ܐܢ (!itr.target() && itr.exists()) r.push_back(itr.filename());
    }
    ܦܢܐ r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ܐܢ (ofp->fail()) v3fatal("Can't write file: " << filename);

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
        ܐܢ (depfilep->filename() == filename) {
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
    ܐܢ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ܦܢܐ ܕܓܠܐ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        ܐܢ (ignore.empty()) { /*used*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdlineIn);
        ܐܢ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ܦܢܐ ܕܓܠܐ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    ܟܡܐ (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        ܐܢ (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
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
        ܐܢ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ܦܢܐ ܕܓܠܐ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ܐܢ (filename == chkFilename) استمر;

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
        ܐܢ (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ܐܢ (skipHashing) ܦܢܐ ܕܓܠܐ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ܐܢ (chkHash == "unhashed") ܦܢܐ ܕܓܠܐ;

            VHashSha512 curHash;
            curHash.insertFile(chkFilename);
            ܐܢ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ܦܢܐ ܕܓܠܐ;
            }
        }
    }
    ܦܢܐ ܫܪܪܐ;
}

//######################################################################
// V3File

فراغ V3File::addSrcDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
فراغ V3File::addTgtDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { ܦܢܐ dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ܦܢܐ dependImp.checkTimes(filename, cmdlineIn);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    ܐܢ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = ܕܓܠܐ;
    ܐܢ (!s_created) {
        s_created = ܫܪܪܐ;
        V3Os::createDir(v3Global.opt.makeDir());
        ܐܢ (v3Global.opt.hierTop()) V3Os::createDir(v3Global.opt.hierTopDataDir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = ܕܓܠܐ;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ܐܚܪܢܐ
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#endif
    منطقي m_pidExited = ܕܓܠܐ;  //  waitpid() reaped the filter process
    صحيح m_pidStatus = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        ܐܢ (m_pid) {
            ܦܢܐ readContentsFilter(filename, outl);
        } ܐܚܪܢܐ {
            ܦܢܐ readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        ܐܢ (fd < 0) ܦܢܐ ܕܓܠܐ;
        m_readEof = ܕܓܠܐ;
        readBlocks(fd, -1, outl);
        close(fd);
        ܦܢܐ ܫܪܪܐ;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string line = readFilterLine();
        ܐܢ (line.find("Content-Length") != string::npos) {
            صحيح len = 0;
            sscanf(line.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            ܦܢܐ ܫܪܪܐ;
        } ܐܚܪܢܐ {
            ܐܢ (line != "") v3error("--pipe-filter protocol error, unexpected: " << line);
            ܦܢܐ ܕܓܠܐ;
        }
#ܐܚܪܢܐ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ܦܢܐ ܕܓܠܐ;
#endif
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#ifdef INFILTER_PIPE
        ܐܢ (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = ܫܪܪܐ;
            m_pidExited = ܫܪܪܐ;
        }
#endif
    }

    فراغ readBlocks(صحيح fd, صحيح size, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ܟܡܐ (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ܐܢ (size > 0 && size < todo) todo = size;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ܐܢ (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } ܐܚܪܢܐ ܐܢ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ܕܓܠܐ);
                V3Os::u_sleep(1000);
                استمر;
            } ܐܚܪܢܐ {
                m_readEof = ܫܪܪܐ;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string readFilterLine() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string line;
        ܟܡܐ (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            line += onechar;
            ܐܢ (onechar == "\n") {
                ܐܢ (line == "\n") {
                    line = "";
                    استمر;
                } ܐܚܪܢܐ {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << line);
        ܦܢܐ line;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& out) {
        ܐܢ (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            ܐܢ (out[out.length() - 1] != '\n') cout << '\n';
        }
        ܐܢ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ܫܪܪܐ;
            stop();
        }
        unsigned offset = 0;
        ܟܡܐ (!m_readEof && out.length() > offset) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ܐܢ (got > 0) {
                offset += got;
            } ܐܚܪܢܐ ܐܢ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ܕܓܠܐ);
                V3Os::u_sleep(1000);
                استمر;
            } ܐܚܪܢܐ {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ start(ثابت string& command) {
        ܐܢ (command == "") {
            m_pid = 0;  // Disabled
        } ܐܚܪܢܐ {
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

        ܐܢ (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ܐܢ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        ܐܢ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ܐܢ (pid == 0) {  // Child
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
        } ܐܚܪܢܐ {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ܕܓܠܐ;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ܕܓܠܐ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ܐܚܪܢܐ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    فراغ stop() {
        ܐܢ (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ܫܪܪܐ);
        ܐܢ (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ܐܚܪܢܐ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

محمي:
    friend صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت auto it = m_contentsMap.find(filename);
        ܐܢ (it != m_contentsMap.end()) {
            outl.push_back(it->second);
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (!readContents(filename, outl)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ܦܢܐ ܫܪܪܐ;
    }
    ساكن size_t listSize(ثابت StrList& sl) {
        size_t result = 0;
        لكل (ثابت string& i : sl) result += i.length();
        ܦܢܐ result;
    }
    ساكن string listString(ثابت StrList& sl) {
        string result;
        لكل (ثابت string& i : sl) result += i;
        ܦܢܐ result;
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
    ܐܢ (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ܦܢܐ m_impp->readWholefile(filename, outl);
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
    ܦܢܐ table;
}();

ثابت std::string& V3OutFormatter::indentSpaces(صحيح num) {
    // Indent the specified number of spaces.
    ܦܢܐ s_indentSpaces[std::max(0, std::min(num, MAXSPACE))];
}

منطقي V3OutFormatter::tokenMatch(ثابت حرف* cp, ثابت حرف* cmp) {
    ܟܡܐ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ܐܢ (*cmp) ܦܢܐ ܕܓܠܐ;
    ܐܢ (*cp && !std::isspace(*cp)) ܦܢܐ ܕܓܠܐ;
    ܦܢܐ ܫܪܪܐ;
}

منطقي V3OutFormatter::tokenStart(ثابت حرف* cp) {
    ܦܢܐ (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

منطقي V3OutFormatter::tokenEnd(ثابت حرف* cp) {
    ܦܢܐ (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

منطقي V3OutFormatter::tokenNotStart(ثابت حرف* cp) {
    ܦܢܐ (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

صحيح V3OutFormatter::endLevels(ثابت حرف* strg) {
    صحيح levels = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        ܟܡܐ (std::isspace(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            ܦܢܐ 0;
        حالة '#':  // Preproc directive
            ܐܢ (m_lang == LA_C) ܦܢܐ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::isalnum(*mp); ++mp) {}
            ܐܢ (mp[0] == ':' && mp[1] != ':') ܦܢܐ (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': levels -= m_blockIndent; اكسر;
        حالة '<':
            ܐܢ (m_lang == LA_XML) {
                ܐܢ (cp[1] == '/') levels -= m_blockIndent;
            }
            اكسر;
        حالة 'e':
            ܐܢ (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: ܦܢܐ levels;  // Letter
        }
    }
    ܦܢܐ levels;
}

فراغ V3OutFormatter::putns(ثابت AstNode* nodep, ثابت حرف* strg) {
    ܐܢ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ܦܢܐ;
    }

    ثابت منطقي putNodeDecoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && FileLine::builtInFilename() != nodep->fileline()->filename();

    ܐܢ (m_prependIndent && ((strg[0] && strg[0] != '\n') || putNodeDecoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = ܕܓܠܐ;
    }

    ܐܢ (putNodeDecoration) {
        ثابت FileLine* ثابت flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ثابت std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    منطقي notstart = ܕܓܠܐ;
    منطقي wordstart = ܫܪܪܐ;
    منطقي equalsForBracket = ܕܓܠܐ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ܐܢ (std::isalpha(*cp)) {
            ܐܢ (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = ܫܪܪܐ;
            ܐܢ (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            ܐܢ (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ثابت منطقي wasInBackslash = m_inBackslash;
        m_inBackslash = ܕܓܠܐ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = ܫܪܪܐ;
            ܐܢ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ܫܪܪܐ;
            } ܐܚܪܢܐ {
                m_prependIndent = ܕܓܠܐ;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = ܫܪܪܐ; اكسر;
        حالة '\t': wordstart = ܫܪܪܐ; اكسر;
        حالة '"':
            wordstart = ܕܓܠܐ;
            ܐܢ (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            اكسر;
        حالة '\\':
            wordstart = ܕܓܠܐ;
            ܐܢ (!wasInBackslash) m_inBackslash = ܫܪܪܐ;
            اكسر;
        حالة '/':
            ܐܢ (m_lang == LA_C || m_lang == LA_VERILOG) {
                ܐܢ (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ܟܡܐ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ܐܢ (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            ܐܢ (!m_inStringLiteral) {
                ܐܢ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.push(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            ܐܢ (!m_inStringLiteral) {
                ܐܢ (m_bracketLevel > 0) {
                    m_parenVec.pop();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            ܐܢ (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.push(m_column);
            }
            اكسر;
        حالة ')':
            ܐܢ (!m_inStringLiteral) {
                ܐܢ (!m_parenVec.empty()) m_parenVec.pop();
                indentDec();
            }
            اكسر;
        حالة '<':
            ܐܢ (m_lang == LA_XML) {
                ܐܢ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ܐܚܪܢܐ ܐܢ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ܐܚܪܢܐ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            ܐܢ (m_lang == LA_XML) {
                indentDec();
                ܐܢ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = ܕܓܠܐ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = ܫܪܪܐ; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = ܕܓܠܐ; اكسر;
        }
    }
}

فراغ V3OutFormatter::putBreakExpr() {
    ܐܢ (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
فراغ V3OutFormatter::putBreak() {
    ܐܢ (!v3Global.opt.decoration()) ܦܢܐ;
    ܐܢ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ܐܢ (exceededWidth()) {
            putcNoTracking('\n');
            ܐܢ (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.top()));
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
    ܐܢ (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3OutFormatter::putsNoTracking(ثابت string& strg) {
    ܐܢ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ܦܢܐ;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3OutFormatter::putcNoTracking(حرف chr) {
    ܐܢ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ܦܢܐ;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = ܫܪܪܐ;
        اكسر;
    حالة '\t': m_column = ((m_column + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_column; اكسر;
    افتراضي:
        ++m_column;
        m_nobreak = ܕܓܠܐ;
        اكسر;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ثابت string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    ܐܢ (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            ܐܢ (c == '"') {
                out += "&quot;"s;
            } ܐܚܪܢܐ ܐܢ (c == '\'') {
                out += "&apos;"s;
            } ܐܚܪܢܐ ܐܢ (c == '<') {
                out += "&lt;"s;
            } ܐܚܪܢܐ ܐܢ (c == '>') {
                out += "&gt;"s;
            } ܐܚܪܢܐ ܐܢ (c == '&') {
                out += "&amp;"s;
            } ܐܚܪܢܐ ܐܢ (std::isprint(c)) {
                out += c;
            } ܐܚܪܢܐ {
                out += "&#"s + cvtToStr(static_cast<unsigned صحيح>(c & 0xff)) + ";";
            }
        }
    } ܐܚܪܢܐ {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            ܐܢ (c == '\\' || c == '"') {
                out += "\\"s + c;
            } ܐܚܪܢܐ ܐܢ (c == '\n') {
                out += "\\n";
            } ܐܚܪܢܐ ܐܢ (c == '\r') {
                out += "\\r";
            } ܐܚܪܢܐ ܐܢ (c == '\t') {
                out += "\\t";
            } ܐܚܪܢܐ ܐܢ (std::isprint(c)) {
                out += c;
            } ܐܚܪܢܐ {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    ܦܢܐ out;
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
    ܐܢ ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    ܐܢ (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

فراغ V3OutFile::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ܫܪܪܐ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        ܐܢ (!std::isalnum(c)) c = '_';
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
        ܦܢܐ s;
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
        ܐܢ (!v3Global.opt.protectIds()) ܦܢܐ old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto it = m_nameMap.find(old);
        ܐܢ (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ܐܚܪܢܐ {
            m_nameMap.emplace(old, old);
            m_newIdSet.insert(old);
        }
        ܦܢܐ old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ܐܢ (!v3Global.opt.protectIds() || old.empty() || !doIt) ܦܢܐ old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto pair = m_nameMap.emplace(old, "");
        ܐܢ (pair.second) {
            string out;
            ܐܢ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } ܐܚܪܢܐ {
                VHashSha512 digest{v3Global.opt.protectKeyDefaulted()};
                digest.insert(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (size_t len = 6; len < out.size() - 3; len += 3) {
                    ثابت string tryout = out.substr(0, len);
                    ܐܢ (m_newIdSet.insert(tryout).second) {
                        out = tryout;
                        اكسر;
                    }
                }
            }
            pair.first->second = out;
        }
        ܦܢܐ pair.first->second;
    }
    string protectWordsIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ܐܢ (!(doIt && v3Global.opt.protectIds())) ܦܢܐ old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        ܟܡܐ (ܫܪܪܐ) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            ܐܢ (pos == string::npos) اكسر;
            out += protectIf(old.substr(start, pos - start), ܫܪܪܐ) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), ܫܪܪܐ);
        ܦܢܐ out;
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
        ܐܢ (trypos != string::npos) {
            ܐܢ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& in) VL_MT_SAFE {
    ܦܢܐ v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
    ܦܢܐ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protectWordsIf(ثابت string& old, منطقي doIt) VL_MT_SAFE {
    ܦܢܐ VIdProtectImp::singleton().protectWordsIf(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
