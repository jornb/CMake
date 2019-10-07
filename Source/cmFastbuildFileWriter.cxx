/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFastbuildFileWriter.h"

cmFastbuildFileWriter::cmFastbuildFileWriter(const std::string& filename)
{
  file.Open(filename);
  file.SetCopyIfDifferent(true);
}

cmFastbuildFileWriter::~cmFastbuildFileWriter()
{
  file.Close();
}

void cmFastbuildFileWriter::WriteSingleLineComment(const std::string& comment)
{
  file << currentIndent << "; " << comment << "\n";
}
