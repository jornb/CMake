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

std::vector<std::string> GetLastExecutedDependencySet(
  const cmFastbuildFileWriter::Target& target)
{
  if (!target.GetPostBuildEvents().empty())
    return { target.GetPostBuildEvents().back().Name };

  if (target.HasLibrary)
    return { target.GetLibrary().Name };

  if (!target.GetPreLinkEvents().empty())
    return { target.GetPreLinkEvents().back().Name };

  if (!target.GetObjectLists().empty()) {
    std::vector<std::string> result;
    for (const auto& ol : target.GetObjectLists()) {
      result.push_back(ol.Alias);
    }
    return result;
  }

  if (!target.GetPreBuildEvents().empty())
    return { target.GetPreBuildEvents().back().Name };

  return {};
}

std::vector<std::vector<std::string>*> GetFirstExecutedPreBuildDependenciesSet(
  cmFastbuildFileWriter::Target& target)
{
  if (!target.GetPreBuildEvents().empty()) {
    return { &target.GetPreBuildEvents().front().PreBuildDependencies };
  }

  if (!target.GetObjectLists().empty()) {
    std::vector<std::vector<std::string>*> result;
    for (auto& ol : target.GetObjectLists()) {
      result.push_back(&ol.PreBuildDependencies);
    }
    return result;
  }

  if (!target.GetPreLinkEvents().empty())
    return { &target.GetPreLinkEvents().front().PreBuildDependencies };

  if (target.HasLibrary)
    return { &target.GetLibrary().PreBuildDependencies };

  if (!target.GetPostBuildEvents().empty())
    return { &target.GetPostBuildEvents().front().PreBuildDependencies };

  return {};
}

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

  if (!objectList.PreBuildDependencies.empty()) {
    file << currentIndent << ".PreBuildDependencies = ";
    WriteArray(objectList.PreBuildDependencies);
  }

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
  PushFunctionCall(library.Type, library.Name);

  if (library.Type == "Library") {
    WriteVariable("Librarian", library.Linker);
    WriteVariable("LibrarianOptions", library.LinkerOptions);
    WriteVariable("LibrarianOutput", library.LinkerOutput);

    // Fastbuild requires a compiler to be defined for linking.
    // We'll just use a dummy compiler
    WriteVariable("Compiler", library.DummyCompiler);
    WriteVariable("CompilerOptions", "-c \"%1\" \"%2\"");
    WriteVariable("CompilerOutputPath", "/dummy/");
  } else {
    // Exe or DLL
    WriteVariable("Linker", library.Linker);
    WriteVariable("LinkerOptions", library.LinkerOptions);
    WriteVariable("LinkerOutput", library.LinkerOutput);
  }

  if (library.Type == "Library") {
    file << currentIndent << ".LibrarianAdditionalInputs = ";
  } else {
    file << currentIndent << ".Libraries = ";
  }
  WriteArray(library.Libraries);

  if (!library.PreBuildDependencies.empty()) {
    file << currentIndent << ".PreBuildDependencies = ";
    WriteArray(library.PreBuildDependencies);
  }

  PopFunctionCall();
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

void cmFastbuildFileWriter::WriteVariable(const std::string& name,
                                          const char* string_literal_argument,
                                          bool convertPaths)
{
  file << currentIndent << "." << name << " = '";
  if (convertPaths) {
    std::string tmp = string_literal_argument;
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

cmFastbuildFileWriter::Library& cmFastbuildFileWriter::Target::MakeLibrary()
{
  HasLibrary = true;
  Library = {};
  Library.Name = Name + "_" + "Library";
  return Library;
}

cmFastbuildFileWriter::ObjectList&
cmFastbuildFileWriter::Target::MakeObjectList()
{
  ObjectLists.emplace_back();
  auto& ol = ObjectLists.back();
  ol.Alias = Name + "_ObjectList_" + std::to_string(ObjectLists.size() - 1);
  return ol;
}

cmFastbuildFileWriter::Exec& cmFastbuildFileWriter::Target::MakePreBuildEvent()
{
  PreBuildEvents.emplace_back();
  auto& e = PreBuildEvents.back();
  e.Name =
    Name + "_PreBuildEvent_" + std::to_string(PreBuildEvents.size() - 1);
  return e;
}

cmFastbuildFileWriter::Exec& cmFastbuildFileWriter::Target::MakePreLinkEvent()
{
  PreLinkEvents.emplace_back();
  auto& e = PreLinkEvents.back();
  e.Name = Name + "_PreLinkEvent_" + std::to_string(PreLinkEvents.size() - 1);
  return e;
}

cmFastbuildFileWriter::Exec&
cmFastbuildFileWriter::Target::MakePostBuildEvent()
{
  PostBuildEvents.emplace_back();
  auto& e = PostBuildEvents.back();
  e.Name =
    Name + "_PostBuildEvent_" + std::to_string(PostBuildEvents.size() - 1);
  return e;
}

cmFastbuildFileWriter::Alias cmFastbuildFileWriter::Target::MakeAlias() const
{
  Alias alias;
  alias.Name = Name;

  for (const auto& x : PreBuildEvents)
    alias.Targets.push_back(x.Name);
  for (const auto& x : ObjectLists)
    alias.Targets.push_back(x.Alias);
  for (const auto& x : PreLinkEvents)
    alias.Targets.push_back(x.Name);
  if (HasLibrary)
    alias.Targets.push_back(Library.Name);
  for (const auto& x : PostBuildEvents)
    alias.Targets.push_back(x.Name);

  return alias;
}

void cmFastbuildFileWriter::Target::ComputeDummyOutputPaths(
  const std::string& root)
{
  auto helper = [&root](auto& items) {
    for (auto& element : items) {
      if (element.ExecOutput.empty()) {
        element.ExecOutput = root + "/" + element.Name + ".txt";
        element.ExecUseStdOutAsOutput = true;
        //element.ExecAlways = true;
      }
    }
  };

  helper(PreBuildEvents);
  helper(PreLinkEvents);
  helper(PostBuildEvents);
}

void cmFastbuildFileWriter::Target::ComputeInternalDependencies()
{
  // Library depends on all object lists
  if (HasLibrary && !ObjectLists.empty()) {
    for (const auto& ol : ObjectLists) {
      Library.Libraries.push_back(ol.Alias);
    }
  }

  // All events depend on the previous events within the same group
  auto setupInternalEventDependencies = [](std::vector<Exec>& events) {
    for (size_t i = 1; i < events.size(); ++i) {
      events[i].PreBuildDependencies.push_back(events[i - 1].Name);
    }
  };
  setupInternalEventDependencies(PreBuildEvents);
  setupInternalEventDependencies(PreLinkEvents);
  setupInternalEventDependencies(PostBuildEvents);

  // All ObjectLists depend on the last pre-build event
  if (!PreBuildEvents.empty()) {
    for (auto& ol : ObjectLists)
      ol.PreBuildDependencies.push_back(PreBuildEvents.back().Name);
  }

  // Setup dependencies for the pre-link events. Note that we only need to set
  // this up for the first event, since all following events are internally
  // dependent.
  if (!PreLinkEvents.empty()) {
    if (!ObjectLists.empty()) {
      // We have object lists, let the first pre-link event depend on all of
      // them
      for (const auto& ol : ObjectLists) {
        PreLinkEvents.front().PreBuildDependencies.push_back(ol.Alias);
      }
    } else if (!PreBuildEvents.empty()) {
      // We don't have object lists, but we do have pre-build events. Let the
      // first pre-link event depend on the last pre-build event.
      PreLinkEvents.front().PreBuildDependencies.push_back(
        PreBuildEvents.back().Name);
    }
  }

  // Note: Library automatically depends on object lists in fasbuild, but we
  // need to setup the dependency on pre-link events
  if (HasLibrary && !PreLinkEvents.empty()) {
    Library.PreBuildDependencies.push_back(PreLinkEvents.back().Name);
  }

  // If we don't have any post-build events, we're done already
  if (PostBuildEvents.empty())
    return;

  // Try to depend on the library
  if (HasLibrary) {
    PostBuildEvents.front().PreBuildDependencies.push_back(Library.Name);
    return;
  }

  // Try to depend on the last pre-link event
  if (!PreLinkEvents.empty()) {
    PostBuildEvents.front().PreBuildDependencies.push_back(
      PreLinkEvents.back().Name);
    return;
  }

  // Try to depend on all object lists
  if (!ObjectLists.empty()) {
    for (const auto& ol : ObjectLists)
      PostBuildEvents.front().PreBuildDependencies.push_back(ol.Alias);
    return;
  }

  // Try to depend on the last pre-build event
  if (!PreBuildEvents.empty()) {
    PostBuildEvents.front().PreBuildDependencies.push_back(
      PreBuildEvents.back().Name);
    return;
  }

  // Nothing else to depend on, we must contain exclusively a post-build event
  // and nothing else
}

void cmFastbuildFileWriter::Target::AddDependency(const Target& dependency)
{
  // If we both have libraries, assume this is a link-level dependency.
  // Note: We use .Libraries and not PreBuildDependencies so that Fastbuild
  // will know to rebuild us when the dependency changes
  if (HasLibrary && dependency.HasLibrary &&
      !dependency.Library.LinkerDependencyOuptut.empty()) {
    auto tmp = dependency.Library.LinkerDependencyOuptut;
    cmSystemTools::ConvertToOutputSlashes(tmp);
    Library.Libraries.push_back(tmp);
  }

  // Simple case of no build events, this is only a link-level dependency
  if (!HasBuildEvents() && !dependency.HasBuildEvents()) {
    return;
  }

  // If we have build events, we have a choice on whether to allow some
  // processing, e.g. our object lists, while some build events are running.
  //
  // Because the build events could output or copy some files that we depend
  // on, we will assume the pessimistic/conservative approach of always waiting
  // for all build events of the dependency
  //
  // To do this, we add the last executed alias from the dependency to our
  // first executed alias. Note that we must handle the case where we depend on
  // an alias set, e.g. multiple objectlists, and not just a single alias.

  auto dependencies = GetLastExecutedDependencySet(dependency);
  auto dependees = GetFirstExecutedPreBuildDependenciesSet(*this);
  for (auto& dependee : dependees)
    for (const auto& d : dependencies)
      dependee->push_back(d);
}
