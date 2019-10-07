/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFastbuildFileWriter_h
#define cmFastbuildFileWriter_h

#include "cmGeneratedFileStream.h"

/** \class cmFastbuildFileWriter
 * \brief Handler for writing Fastbuild .bff files.
 */
class cmFastbuildFileWriter
{
public:
  /** Opens a file for writing at the given path. */
  cmFastbuildFileWriter(const std::string& filename);

  /** Closes the file */
  ~cmFastbuildFileWriter();

  void WriteSingleLineComment(const std::string& comment);

private:
  cmGeneratedFileStream file;

  std::string currentIndent;
};

#endif
