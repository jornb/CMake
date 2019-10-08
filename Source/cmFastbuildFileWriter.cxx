/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFastbuildFileWriter.h"

#include <stdexcept>

#include "cmSystemTools.h"

#ifdef _WIN32
void cmFastbuildFileWriter::GenerateBuildScript(
  const std::string& filePrefix, cmFastbuildFileWriter::Exec& exec,
  const cmCustomCommand& command)
{
  auto filename = filePrefix + ".bat";

  // Write build script
  {
    // Open file
    cmGeneratedFileStream file;
    file.Open(filename);

    file << "REM Auto-generated script file for CMake build event\n\n";
    if (command.GetComment()) {
      file << "REM " << command.GetComment() << "\n\n";
    }
    file << "setlocal\n";
    for (const auto& cmd : command.GetCommandLines()) {
      for (const auto& arg : cmd) {
        file << arg << " ";
      }
      file << "\n";
      file << "if %errorlevel% neq 0 goto :end\n";
    }
    file << ":end\n";
    file << "endlocal & exit /b %errorlevel%\n";

    // Close file
    file.Close();
  }

  // Setup exec
  exec.ExecExecutable = cmSystemTools::FindProgram("cmd.exe");
  exec.ExecArguments.push_back("/C");
  exec.ExecArguments.push_back(filename);
}
#else
// TODO
#endif

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
    WriteArray(compiler.ExtraFiles);
  }

  PopFunctionCall();
}

void cmFastbuildFileWriter::Write(const ObjectList& objectList)
{
  PushFunctionCall("ObjectList", objectList.Alias);
  WriteVariable("Compiler", objectList.Compiler);
  WriteVariable("CompilerOptions", objectList.CompilerOptions);

  auto tmp = objectList.CompilerOutputPath;
  cmSystemTools::ConvertToOutputSlashes(tmp);
  WriteVariable("CompilerOutputPath", tmp);

  // Write files
  file << currentIndent << ".CompilerInputFiles = ";
  WriteArray(objectList.CompilerInputFiles, true);

  PopFunctionCall();
}

void cmFastbuildFileWriter::Write(const Alias& alias)
{
  PushFunctionCall("Alias", alias.Name);
  file << currentIndent << ".Targets  = ";
  WriteArray(alias.Targets);
  PopFunctionCall();
}

void cmFastbuildFileWriter::Write(const Library& library)
{
  // TODO
}

void cmFastbuildFileWriter::Write(const Exec& exec)
{
  PushFunctionCall("Exec", exec.Name);
  WriteVariable("ExecExecutable", exec.ExecExecutable, true);
  WriteVariable("ExecWorkingDir", exec.ExecWorkingDir, true);
  WriteVariable("ExecOutput", exec.ExecOutput, true);
  WriteVariable("ExecUseStdOutAsOutput", exec.ExecUseStdOutAsOutput);
  WriteVariable("ExecAlways", exec.ExecAlways);

  // Write exec arguments as string
  if (!exec.ExecArguments.empty()) {
    file << currentIndent << ".ExecArguments = '";
    for (size_t i = 0; i < exec.ExecArguments.size(); ++i) {
      if (i > 0) {
        file << " ";
      }

	  // TODO: Quote
      file << exec.ExecArguments[i];
    }
    file << "'\n";
  }

  if (!exec.PreBuildDependencies.empty()) {
    file << currentIndent << ".PreBuildDependencies = ";
    WriteArray(exec.PreBuildDependencies);
  }

  PopFunctionCall();
}

void cmFastbuildFileWriter::WriteVariable(
  const std::string& name, const std::string& string_literal_argument,
  bool convertPaths)
{
  file << currentIndent << "." << name << " = '";
  if (convertPaths) {
    auto tmp = string_literal_argument;
    cmSystemTools::ConvertToOutputSlashes(tmp);
    file << tmp;
  } else {
    file << string_literal_argument;
  }
  file << "'\n";
}

void cmFastbuildFileWriter::WriteVariable(const std::string& name,
                                          bool boolean_literal)
{
  file << currentIndent << "." << name << " = "
       << (boolean_literal ? "true" : "false") << "\n";
}

void cmFastbuildFileWriter::PushFunctionCall(
  const std::string& function, const std::string& string_literal_argument)
{
  file << currentIndent << function << "(";
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

void cmFastbuildFileWriter::WriteArray(const std::vector<std::string>& values,
                                       bool convertPaths, bool quote)
{
  PushScope("{");
  for (size_t i = 0; i < values.size(); ++i) {
    file << currentIndent;
    if (quote)
      file << "'";
    if (convertPaths) {
      auto tmp = values[i];
      cmSystemTools::ConvertToOutputSlashes(tmp);
      file << tmp;
    } else {
      file << values[i];
    }
    if (quote)
      file << "'";

    if (i < values.size() - 1)
      file << ",";

    file << "\n";
  }
  PopScope("}");
}

void cmFastbuildFileWriter::Target::ComputeNames()
{
  int i = 0;

  for (auto& element : PreBuildEvents)
    element.Name = Name + std::to_string(i++);

  for (auto& element : ObjectLists)
    element.Alias = Name + std::to_string(i++);

  for (auto& element : PreLinkEvents)
    element.Name = Name + std::to_string(i++);

  if (HasLibrary)
    Library.Name = Name + std::to_string(i++);

  for (auto& element : PostBuildEvents)
    element.Name = Name + std::to_string(i++);
}

void cmFastbuildFileWriter::Target::ComputeDummyOutputPaths(
  const std::string& root)
{
  auto helper = [&root](auto& items) {
    for (auto& element : items) {
      if (element.ExecOutput.empty()) {
        element.ExecOutput = root + "/" + element.Name + ".txt";
        element.ExecUseStdOutAsOutput = true;
        element.ExecAlways = true;
      }
    }
  };

  helper(PreBuildEvents);
  helper(PreLinkEvents);
  helper(PostBuildEvents);
}

void cmFastbuildFileWriter::Target::ComputeInternalDependencies()
{
}

std::string cmFastbuildFileWriter::Target::GetLastExecutedAlias() const
{
  if (!PostBuildEvents.empty())
    return PostBuildEvents.back().Name;

  if (HasLibrary)
    return Library.Name;

  if (!PreLinkEvents.empty())
    return PreLinkEvents.back().Name;

  if (!ObjectLists.empty())
    return ObjectLists.back().Alias;

  if (!PreBuildEvents.empty())
    return PreBuildEvents.back().Name;

  return "";
}
