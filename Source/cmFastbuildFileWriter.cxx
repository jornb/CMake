/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFastbuildFileWriter.h"

#include <stdexcept>

#include "cmSystemTools.h"

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

void cmFastbuildFileWriter::Write(const Compiler& compiler)
{
  std::string compilerPath =
    cmSystemTools::GetFilenamePath(compiler.Executable);
  std::string compilerFile =
    "$CompilerRoot$/" + cmSystemTools::GetFilenameName(compiler.Executable);

  cmSystemTools::ConvertToOutputSlashes(compilerPath);
  cmSystemTools::ConvertToOutputSlashes(compilerFile);

  PushFunctionCall("Compiler", compiler.Name);
  WriteVariable("CompilerRoot", compilerPath);
  WriteVariable("Executable", compilerFile);

  // Write additional files
  if (!compiler.ExtraFiles.empty()) {
    file << currentIndent << ".ExtraFiles = ";
    PushScope("{");
    for (size_t i = 0; i < compiler.ExtraFiles.size(); ++i) {
      file << currentIndent << "'" << compiler.ExtraFiles[i] << "'";

      if (i < compiler.ExtraFiles.size() - 1)
        file << ",";

      file << "\n";
    }
    PopScope("}");
  }

  PopFunctionCall();
}

void cmFastbuildFileWriter::WriteVariable(
  const std::string& name, const std::string& string_literal_argument)
{
  file << currentIndent << "." << name << " = '" << string_literal_argument
       << "'\n";
}

void cmFastbuildFileWriter::PushFunctionCall(
  const std::string& function, const std::string& string_literal_argument)
{
  file << function << "(";
  if (!string_literal_argument.empty()) {
    file << "'" << string_literal_argument << "'";
  }
  file << ")";

  PushScope("{");
}

void cmFastbuildFileWriter::PopFunctionCall()
{
  PopScope("}");
}

void cmFastbuildFileWriter::PushScope(const std::string& delimiter)
{
  file << delimiter << "\n";
  currentIndent.push_back('\t');
}

void cmFastbuildFileWriter::PopScope(const std::string& delimiter)
{
  if (currentIndent.empty()) {
    throw std::runtime_error(
      "Internal CMake error: incorrect use of fastbuild file writer");
  }

  // Remove one \t from currentIndent
  currentIndent.pop_back();

  // Write the ending delimeter on a separate line
  file << currentIndent << delimiter << "\n";
}
