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

public:
  /** Opens a file for writing at the given path. */
  cmFastbuildFileWriter(const std::string& filename);

  /** Closes the file */
  ~cmFastbuildFileWriter();

  void WriteSingleLineComment(const std::string& comment);
  void Write(const Compiler& compiler);
  void WriteVariable(const std::string& name,
                     const std::string& string_literal_argument);
  void PushFunctionCall(const std::string& function,
                        const std::string& string_literal_argument = "");
  void PopFunctionCall();

private:
  void PushScope(const std::string& delimiter);
  void PopScope(const std::string& delimiter);

private:
  cmGeneratedFileStream file;

  std::string currentIndent;
};

#endif
