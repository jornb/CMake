/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFastbuildFileWriter_h
#define cmFastbuildFileWriter_h

#include <string>
#include <vector>

#include "cmGeneratedFileStream.h"

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
    std::vector<std::string>
      ExtraFiles; //!< (optional) Additional files (usually dlls) required by
                  //!< the compiler.
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
  };

  //! See http://www.fastbuild.org/docs/functions/objectlist.html
  struct Alias
  {
    std::string Name;
    std::vector<std::string> Targets;
  };

public:
  /** Opens a file for writing at the given path. */
  cmFastbuildFileWriter(const std::string& filename);

  /** Closes the file */
  ~cmFastbuildFileWriter();

  void WriteSingleLineComment(const std::string& comment);
  void Write(const Compiler& compiler);
  void Write(const ObjectList& compiler);
  void Write(const Alias& compiler);
  void WriteVariable(const std::string& name,
                     const std::string& string_literal_argument,
                     bool convertPaths = false);
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
