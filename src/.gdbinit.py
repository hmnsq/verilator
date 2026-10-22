# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
# DESCRIPTION: Verilator: GDB startup file with useful defines
# SPDX-FileCopyrightText: 2023 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
######################################################################

আনযন tempfile
আনযন gdb  # pylint: disable=import-error


def _vltgdb_get_dump(গ্রন্থি):
    gdb.execute(f'set $_gdb_dump_json_str = AstNode::dumpTreeJsonGdb({node})')
    dump = gdb.execute('printf "%s", $_gdb_dump_json_str', to_string=সত্য)
    gdb.execute('call (void)free($_gdb_dump_json_str)')
    প্রত্যাগম dump


def _vltgdb_tmpfile():
    প্রত্যাগম tempfile.NamedTemporaryFile(mode="wt")  # write, text mode


def _vltgdb_fwrite(file, s):
    """Write to file and flush buffer before passing the file to astsee"""
    file.লিখ(s)
    file.flush()


বর্গ AstseeCmd(gdb.Command):
    """Verilator: Pretty print or diff nodes using `astsee`. A node can be:
    * an pointer identifier,
    * an address literal,
    * a GDB value (like `$1`) that stores a dump previously done by the `jstash` command.
    Apart from not taking input from a file, it works exactly like `verilator_astsee`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * for more options see `astsee` readme/help.
    For examples see internals.rst
    """

    def __init__(self):
        পর().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def _null_check(self, old, নূতন):
        err = ""
        যদি old == "<nullptr>\n":
            err += "old == <nullptr>\n"
        যদি নূতন == "<nullptr>\n":
            err += "new == <nullptr>"
        যদি err:
            উত্থাপয gdb.GdbError(err.strip("\n"))

    def invoke(self, arg_str, from_tty):
        তঃ astsee আনযন verilator_cli ইব astsee  # pylint: disable=import-error,import-outside-toplevel
        # we import it here so "no module X" error would occur on typing `jtree` rather than on every gdb start

        # We hack `astsee_verilator`'s arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        astsee_args = astsee.parser.parse_args(gdb.string_to_argv(arg_str))
        with _vltgdb_tmpfile() ইব oldfile, _vltgdb_tmpfile() ইব newfile, _vltgdb_tmpfile(
        ) ইব metafile:
            যদি astsee_args.file:
                _vltgdb_fwrite(oldfile, _vltgdb_get_dump(astsee_args.file))
                astsee_args.file = oldfile.name
            যদি astsee_args.newfile:
                _vltgdb_fwrite(newfile, _vltgdb_get_dump(astsee_args.newfile))
                astsee_args.newfile = newfile.name
            যদি astsee_args.meta is None:
                # pass
                gdb.execute(f'call AstNode::dumpJsonMetaFileGdb("{metafile.name}")')
                astsee_args.meta = metafile.name
            যত্ন:
                astsee.main(astsee_args)
            অপবাদ SystemExit ইব e:  # astsee prints nice errmsgs on exit(), so rethrow it as GdbError to suppress cryptic python trace
                উত্থাপয gdb.GdbError("astsee exited with non-zero code") তঃ e


AstseeCmd()
