/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFastbuildFileWriter_h
#define cmFastbuildFileWriter_h

#include <string>
#include <vector>

#include "cmGeneratedFileStream.h"
#include "cmCustomCommand.h"

/** \class cmFastbuildFileWriter
 * \brief Handler for writing Fastbuild .bff files.
 */
class cmFastbuildFileWriter
{
public:
  //! See http://www.fastbuild.org/docs/functions/compiler.html
  struct Compiler
  {
    std::string Name;
    std::string Executable; //!< Primary compiler executable
    std::string CompilerFamily;
    std::vector<std::string>
      ExtraFiles; //!< (optional) Additional files (usually dlls) required by
                  //!< the compiler.

    std::string Language;
  };

  //! See http://www.fastbuild.org/docs/functions/objectlist.html
  struct ObjectList
  {
    std::string Alias;
    std::string Compiler;           //!< Compiler to use
    std::string CompilerOptions;    //!< Options for compiler
    std::string CompilerOutputPath; //!< Path to store intermediate objects

    //! Explicit array of files to build
    std::vector<std::string> CompilerInputFiles;

    //! (optional) Force targets to be built before this
    std::vector<std::string> PreBuildDependencies;

    //! (optional) Append extension instead of replacing it
    bool CompilerOutputKeepBaseExtension = true;
  };

  //! See http://www.fastbuild.org/docs/functions/objectlist.html
  struct Alias
  {
    std::string Name;
    std::vector<std::string> Targets;
  };

  //! See http://www.fastbuild.org/docs/functions/exec.html
  struct Exec
  {
    std::string Name;

    //! Executable to run
    std::string ExecExecutable;

    //! Output file generated by executable
    std::string ExecOutput;

    //! (optional) Working dir to set for executable
    std::string ExecWorkingDir;

    //! (optional) Arguments to pass to executable
    std::vector<std::string> ExecArguments;

    //! (optional) Write the standard output from the executable to the output
    //! file
    bool ExecUseStdOutAsOutput = false;

    //! (optional) Run the executable even if inputs have not changed
    bool ExecAlways = false;

    //! (optional) Force targets to be built before this
    std::vector<std::string> PreBuildDependencies;

    //! Create a no-operation target. Note that all fastbuild Execs must have a
    //! well-defined output file, even if it is not used. This file will be put
    //! in the given directory.
    static Exec Noop(const std::string& outputDir);
  };

  //! See http://www.fastbuild.org/docs/functions/executable.html and http://www.fastbuild.org/docs/functions/dll.html
  struct Library
  {
    std::string Name;

    std::string Type;

    //! Linker executable to use
    std::string Linker;

    //! Output from linker
    std::string LinkerOutput;

    //! Options to pass to linker
    std::string LinkerOptions;

	//! Output that dependent targets depend on.
	//! For static libraries, this is the regular .lib file.
	//! For dynamic libraries, this is the import .lib file.
	//! For executables, this is empty.
	std::string LinkerDependencyOuptut;

    //! Libraries to link into the binary. Can be other targets.
    std::vector<std::string> Libraries;

    //! (optional) Force targets to be built before this
    std::vector<std::string> PreBuildDependencies;

	//! Dummy compiler
	std::string DummyCompiler;
  };

  struct Target
  {
    explicit Target(const std::string& name)
      : Name(name)
    {
    }

    const Library& GetLibrary() const { return Library; }
    const std::vector<ObjectList>& GetObjectLists() const
    {
      return ObjectLists;
    }
    const std::vector<Exec>& GetPreBuildEvents() const
    {
      return PreBuildEvents;
    }
    const std::vector<Exec>& GetPreLinkEvents() const { return PreLinkEvents; }
    const std::vector<Exec>& GetPostBuildEvents() const
    {
      return PostBuildEvents;
    }

    Library& GetLibrary() { return Library; }
    std::vector<ObjectList>& GetObjectLists() { return ObjectLists; }
    std::vector<Exec>& GetPreBuildEvents() { return PreBuildEvents; }
    std::vector<Exec>& GetPreLinkEvents() { return PreLinkEvents; }
    std::vector<Exec>& GetPostBuildEvents() { return PostBuildEvents; }

    bool HasBuildEvents() const
    {
      return !PreBuildEvents.empty() && !PreLinkEvents.empty() &&
        !PostBuildEvents.empty();
    }

    Library& MakeLibrary();
    ObjectList& MakeObjectList();
    Exec& MakePreBuildEvent();
    Exec& MakePreLinkEvent();
    Exec& MakePostBuildEvent();

    Alias MakeAlias() const;

    void ComputeDummyOutputPaths(const std::string& root);
    void ComputeInternalDependencies();

    //! \briefAdd a utility dependency between two targets. `this` depends on
    //! `dependency`.
    //! This method only updates the `PreBuildDependencies` fields to ensure
    //! the build order is correct.
    void AddUtilDependency(const Target& dependency);

    //! \briefAdd a dependency between two targets. `this` depends on
    //! `dependency`.
    //!
    //! Note that all dependencies between the libraries and object lists etc.
    //! have already been accounted for. This method only updates the
    //! `PreBuildDependencies` fields to ensure the build order is correct.
    void AddLinkDependency(const Target& dependency);

    std::string Name;
    bool HasLibrary = false;

  private:
    std::vector<ObjectList> ObjectLists;
    Library Library;
    std::vector<Exec> PreBuildEvents;
    std::vector<Exec> PreLinkEvents;
    std::vector<Exec> PostBuildEvents;
  };

  static void GenerateBuildScript(const std::string& filePrefix,
                                  cmFastbuildFileWriter::Exec& exec,
                                  const cmCustomCommand& command,
                                  const std::string& args_replace = "");

public:
  /** Opens a file for writing at the given path. */
  cmFastbuildFileWriter(const std::string& filename);

  /** Closes the file */
  ~cmFastbuildFileWriter();

  void WriteSingleLineComment(const std::string& comment);
  void Write(const Compiler& compiler);
  void Write(const ObjectList& compiler);
  void Write(const Alias& compiler);
  void Write(const Library& library);
  void Write(const Exec& exec);
  void WriteVariable(const std::string& name,
                     const std::string& string_literal_argument,
                     bool convertPaths = false)
  {
    WriteVariable(name, string_literal_argument.c_str(), convertPaths);
  }
  void WriteVariable(const std::string& name,
                     const char *string_literal_argument,
                     bool convertPaths = false);
  void WriteVariable(const std::string& name, bool boolean_literal);
  void PushFunctionCall(const std::string& function,
                        const std::string& string_literal_argument = "");
  void PopFunctionCall();
  void PushScope(const std::string& delimiter);
  void PopScope(const std::string& delimiter);

private:
  void WriteArray(const std::vector<std::string>& values,
                  bool convertPaths = false, bool quote = true);

private:
  cmGeneratedFileStream file;

  std::string currentIndent;
};

#endif
