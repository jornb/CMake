/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalFastbuildGenerator.h"

#include "cmComputeLinkInformation.h"
#include "cmCustomCommandGenerator.h"
#include "cmDocumentationEntry.h"
#include "cmFastbuildLinkLineComputer.h"
#include "cmFastbuildTargetGenerator.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmGlobalGeneratorFactory.h"
#include "cmLocalGenerator.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmake.h"

class 

cmGeneratorTarget::Names GetOutputNames(cmGeneratorTarget* target,
                                        const std::string& config)
{
  if (target->GetType() == cmStateEnums::EXECUTABLE)
    return target->GetExecutableNames(config);
  else
    return target->GetLibraryNames(config);
}

std::string GetCompilePdbPath(cmGeneratorTarget* target,
                              const std::string& config)
{
  // First try to get the explicit PDB directory. Otherwise, use the support
  // directory.
  auto pdbDir = target->GetCompilePDBPath(config);
  if (pdbDir.empty())
    pdbDir = target->GetSupportDirectory();

  return pdbDir + "/" + GetOutputNames(target, config).PDB;
}

std::string GetCompilerFlags(cmLocalCommonGenerator* lg, cmGeneratorTarget* gt,
                             const std::vector<const cmSourceFile*>& sourceFiles,
                             const std::string& language,
                             const std::string& config)
{
  std::string compileFlags = "";
  lg->AddLanguageFlags(compileFlags, gt, language, config);

  lg->AddArchitectureFlags(compileFlags, gt, language, config);

  // Add shared-library flags if needed.
  lg->AddCMP0018Flags(compileFlags, gt, language, config);

  lg->AddVisibilityPresetFlags(compileFlags, gt, language);

  std::vector<std::string> includes;
  lg->GetIncludeDirectories(includes, gt, language, config);

  // Add include directory flags.
  // FIXME const cast are evil
  std::string includeFlags = lg->GetIncludeFlags(
    includes, gt, language,
    language == "RC" ? true : false, // full include paths for RC
    // needed by cmcldeps
    false, config);

  lg->AppendFlags(compileFlags, includeFlags);

  // Append old-style preprocessor definition flags.
  lg->AppendFlags(compileFlags, lg->GetMakefile()->GetDefineFlags());

  // Add target-specific flags.
  // FIXME const cast are evil
  lg->AddCompileOptions(compileFlags, gt, language, config);

  for (const auto source : sourceFiles) {
    auto sourceFlags = source->GetProperty("COMPILE_FLAGS");
    if (sourceFlags) {
      lg->AppendFlags(compileFlags, source->GetProperty("COMPILE_FLAGS"));
    }
  }

  return compileFlags;
}

std::string GetCompileDefines(cmLocalCommonGenerator* lg,
                              cmGeneratorTarget* gt,
                              const std::vector<const cmSourceFile*>& sourceFiles,
                              const std::string& config,
                              const std::string& language)
{
  std::set<std::string> defines;

  // Add the export symbol definition for shared library objects.
  auto exportMacro = gt->GetExportMacro();
  if (exportMacro) {
    lg->AppendDefines(defines, *exportMacro);
  }

  // Add preprocessor definitions for this target and configuration.
  lg->GetTargetDefines(gt, config, language, defines);

  // Add compile definitions set on individual source files
  for (const auto source : sourceFiles) {
    auto compileDefinitions = source->GetProperty("COMPILE_DEFINITIONS");
    if (compileDefinitions)
      lg->AppendDefines(defines, compileDefinitions);

    compileDefinitions = source->GetProperty("COMPILE_DEFINITIONS_" +
                                             cmSystemTools::UpperCase(config));
    if (compileDefinitions)
      lg->AppendDefines(defines, compileDefinitions);
  }

  // Add a definition for the configuration name.
  lg->AppendDefines(defines, "CMAKE_INTDIR=\"" + config + "\"");

  std::string definesString;
  lg->JoinDefines(defines, definesString, language);

  return definesString;
}
std::string GetCompileArguments(cmGeneratorTarget* target,
                                const std::string& manifests,
                                const std::string& language,
                                const std::string& config)
{
  auto localCommonGenerator =
    static_cast<cmLocalCommonGenerator*>(target->LocalGenerator);

  const auto& targetName = target->GetName();
  const auto targetTypeStr = cmState::GetTargetTypeName(target->GetType());

  auto pdbPath = GetCompilePdbPath(target, config);
  cmSystemTools::ConvertToOutputSlashes(pdbPath);

  cmRulePlaceholderExpander::RuleVariables compileObjectVars;
  compileObjectVars.CMTargetName = targetName.c_str();
  compileObjectVars.CMTargetType = targetTypeStr;
  compileObjectVars.Language = language.c_str();
  compileObjectVars.Source = "\"%1\"";
  compileObjectVars.Object = "\"%2\"";
  compileObjectVars.ObjectDir = "$TargetOutputDir$";
  compileObjectVars.ObjectFileDir = "";
  compileObjectVars.Flags = "";
  compileObjectVars.Includes = "";
  compileObjectVars.Manifests = manifests.c_str();
  compileObjectVars.Defines = "";
  compileObjectVars.TargetCompilePDB = pdbPath.c_str();

  // Get all commands necessary to compile objects
  std::string compileCmdVariable =
    localCommonGenerator->GetMakefile()->GetRequiredDefinition(
      "CMAKE_" + language + "_COMPILE_OBJECT");
  std::vector<std::string> compileCmds;
  cmExpandList(compileCmdVariable, compileCmds);

  // We don't know how to handle more than one command.
  // We expect a single command which starts with the CMAKE_CXX_COMPILER.
  if (compileCmds.size() != 1 ||
      compileCmds[0].rfind("<CMAKE_CXX_COMPILER> ", 0) == std::string::npos)
    throw std::runtime_error(
      "Internal CMake error: Fastbuild expected a single command for object "
      "compilation starting with <CMAKE_CXX_COMPILER>");

  // Split the <CMAKE_CXX_COMPILER> off the command
  auto compileCommand =
    compileCmds[0].substr(std::string("<CMAKE_CXX_COMPILER> ").length());

  // Create rule expander in a unique_ptr so it is ensured to be cleaned up
  auto localFastbuildGenerator =
    static_cast<cmLocalFastbuildGenerator*>(localCommonGenerator);
  std::unique_ptr<cmRulePlaceholderExpander> rulePlaceholderExpander{
    localFastbuildGenerator->CreateRulePlaceholderExpander()
  };

  // Expand the compile command
  rulePlaceholderExpander->ExpandRuleVariables(
    localFastbuildGenerator, compileCommand, compileObjectVars);

  return compileCommand;
}

cmFastbuildFileWriter::ObjectList CreateFastbuildObjectList(
  cmGeneratorTarget* target, const std::string& config,
  const std::string& language)
{
  auto localCommonGenerator =
    static_cast<cmLocalCommonGenerator*>(target->LocalGenerator);

  // Collect all source files of this language
  std::vector<const cmSourceFile*> languageSourceFiles;
  std::vector<std::string> sourceFiles;
  for (const auto& sourceFile : target->GetSourceFiles(config)) {
    if (sourceFile.Value->GetLanguage() == language)
      languageSourceFiles.push_back(sourceFile.Value);
    sourceFiles.push_back(sourceFile.Value->GetLocation().GetFullPath());
  }

  std::string manifests;
  target->GetManifests(languageSourceFiles, config);

  cmFastbuildFileWriter::ObjectList objectList;
  objectList.Compiler = "Compiler_" + language;
  objectList.CompilerOptions;
  objectList.CompilerOutputPath = target->GetSupportDirectory() + "/" + config;
  objectList.CompilerInputFiles = sourceFiles;

  // Build compile invocation arguments
  objectList.CompilerOptions += GetCompilerFlags(
    localCommonGenerator, target, languageSourceFiles, language, config);
  objectList.CompilerOptions += " ";
  objectList.CompilerOptions += GetCompileDefines(
    localCommonGenerator, target, languageSourceFiles, language, config);
  objectList.CompilerOptions += " ";
  objectList.CompilerOptions +=
    GetCompileArguments(target, manifests, language, config);

  return objectList;
}

void CreateFastbuildCustomCommands(
  const std::string& scriptFilenamePrefix, const cmCustomCommand& command,
  std::vector<cmFastbuildFileWriter::Exec>& execs)
{
  cmFastbuildFileWriter::Exec exec;
  exec.ExecWorkingDir = command.GetWorkingDirectory();
  cmFastbuildFileWriter::GenerateBuildScript(
    scriptFilenamePrefix + "_" + std::to_string(execs.size()), exec, command);

  execs.push_back(exec);
}

std::vector<cmFastbuildFileWriter::Target> CreateFastbuildTargets(
  cmMakefile* makefile, const std::set<cmGeneratorTarget*>& targets)
{
  std::vector<cmFastbuildFileWriter::Target> fastbuildTargets;

  // Get all configurations
  std::vector<std::string> configs;
  makefile->GetConfigurations(configs, false);

  // Write object file list for each language and each configuration
  for (const auto& config : configs) {
    for (const auto& target : targets) {
      auto targetType = target->GetType();

      // Initialize target
      cmFastbuildFileWriter::Target fbTarget;
      fbTarget.Name = target->GetName() + "_" + config;

      // Handle "regular" code (executable, library, module, object library)
      if (targetType < cmStateEnums::UTILITY) {
        // Get all languages (e.g. CXX and or C) in target
        auto languages = fastbuild::detail::DetectTargetLanguages({ target });

        // Create one object list per language
        for (const auto& language : languages) {
          fbTarget.ObjectLists.push_back(
            CreateFastbuildObjectList(target, config, language));
        }

        // TODO: Add library
      }

      // Add build events
      for (const auto& cmd : target->GetPreBuildCommands())
        CreateFastbuildCustomCommands(makefile->GetHomeOutputDirectory() +
                                        "/pre_build",
                                      cmd, fbTarget.PreBuildEvents);
      for (const auto& cmd : target->GetPreLinkCommands())
        CreateFastbuildCustomCommands(makefile->GetHomeOutputDirectory() +
                                        "/pre_link",
                                      cmd, fbTarget.PreLinkEvents);
      for (const auto& cmd : target->GetPostBuildCommands())
        CreateFastbuildCustomCommands(makefile->GetHomeOutputDirectory() +
                                        "/post_build",
                                      cmd, fbTarget.PostBuildEvents);

      fbTarget.ComputeNames();
      fbTarget.ComputeDummyOutputPaths(makefile->GetHomeOutputDirectory());
      fbTarget.ComputeInternalDependencies();

      fastbuildTargets.push_back(fbTarget);
    }
  }

  return fastbuildTargets;
}

cmGlobalFastbuildGenerator::cmGlobalFastbuildGenerator(cmake* cm)
  : cmGlobalCommonGenerator(cm)
{
#ifdef _WIN32
  cm->GetState()->SetWindowsShell(true);
#endif
  this->FindMakeProgramFile = "CMakeFastbuildFindMake.cmake";
}

cmGlobalGeneratorFactory* cmGlobalFastbuildGenerator::NewFactory()
{
  return new cmGlobalGeneratorSimpleFactory<cmGlobalFastbuildGenerator>();
}

void cmGlobalFastbuildGenerator::GetDocumentation(cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalFastbuildGenerator::GetActualName();
  entry.Brief = "Generates Fastbuild .bff makefiles.";
}

void cmGlobalFastbuildGenerator::AppendDirectoryForConfig(
  const std::string& prefix, const std::string& config,
  const std::string& suffix, std::string& dir)
{
  // TODO
}

void cmGlobalFastbuildGenerator::ComputeTargetObjectDirectory(
  cmGeneratorTarget* gt) const
{
  // TODO
}

cmLinkLineComputer* cmGlobalFastbuildGenerator::CreateLinkLineComputer(
  cmOutputConverter* outputConverter, cmStateDirectory const& stateDir) const
{
  return new cmFastBuildLinkLineComputer(
    outputConverter,
    this->LocalGenerators[0]->GetStateSnapshot().GetDirectory(), this);
}

cmLocalGenerator* cmGlobalFastbuildGenerator::CreateLocalGenerator(
  cmMakefile* makefile)
{
  return new cmLocalFastbuildGenerator(this, makefile);
}

void cmGlobalFastbuildGenerator::EnableLanguage(
  std::vector<std::string> const& lang, cmMakefile* mf, bool optional)
{
  this->cmGlobalGenerator::EnableLanguage(lang, mf, optional);

  // Ensure configuration types is not none. Default to
  // Debug;Release;MinSizeRel;RelWithDebInfo
  if (!mf->GetDefinition("CMAKE_CONFIGURATION_TYPES")) {
    mf->AddCacheDefinition(
      "CMAKE_CONFIGURATION_TYPES", "Debug;Release;MinSizeRel;RelWithDebInfo",
      "Semicolon separated list of supported configuration types, "
      "only supports Debug, Release, MinSizeRel, and RelWithDebInfo, "
      "anything else will be ignored.",
      cmStateEnums::STRING);
  }
}

void cmGlobalFastbuildGenerator::Generate()
{
  // Run the normal generation process
  cmGlobalGenerator::Generate();

  // Creat the top-level fastbuild bff file
  GenerateBffFile();
}

std::vector<cmGlobalGenerator::GeneratedMakeCommand>
cmGlobalFastbuildGenerator::GenerateBuildCommand(
  const std::string& makeProgram, const std::string& projectName,
  const std::string& projectDir, std::vector<std::string> const& targetNames,
  const std::string& config, bool fast, int jobs, bool verbose,
  std::vector<std::string> const& makeOptions)
{
  // TODO

  return {};
}

const char* cmGlobalFastbuildGenerator::GetCMakeCFGIntDir() const
{
  // TODO
  return "FASTBUILD_CONFIG_INT_DIR";
}

std::string cmGlobalFastbuildGenerator::ConvertToFastbuildPath(
  const std::string& path) const
{
  auto ng = static_cast<cmLocalFastbuildGenerator*>(LocalGenerators[0]);
  return ng->MaybeConvertToRelativePath(
    LocalGenerators[0]->GetState()->GetBinaryDirectory(), path);
}

void cmGlobalFastbuildGenerator::GenerateBffFile()
{
  // Get the root local generator
  auto root = static_cast<cmLocalFastbuildGenerator*>(LocalGenerators[0]);

  // Open bff file for writing
  cmFastbuildFileWriter file{ root->GetMakefile()->GetHomeOutputDirectory() +
                              "/fbuild.bff" };

  file.WriteSingleLineComment("This file was auto-generated by cmake.");

  // Collect all targets
  auto targets = DetectTargetGenerators();

  GenerateBffCompilerSection(file, root->GetMakefile(), targets);

  // TODO: Sort the targets based on dependency order

  GenerateBffTargetSection(file, root->GetMakefile(), targets);
}

void cmGlobalFastbuildGenerator::GenerateBffCompilerSection(
  cmFastbuildFileWriter& file, cmMakefile* makefile,
  const std::set<cmGeneratorTarget*>& targets) const
{
  file.WriteSingleLineComment("Compilers");

  // Output compiler for each language
  std::vector<std::string> compiler_languages;
  for (const auto& language : fastbuild::detail::DetectTargetLanguages(targets)) {
    // Get the root location of the compiler
    std::string variableString = "CMAKE_" + language + "_COMPILER";
    std::string executable = makefile->GetSafeDefinition(variableString);
    if (executable.empty()) {
      continue;
    }

    // Remember language for later
    compiler_languages.push_back(language);

    // Output compiler definition
    cmFastbuildFileWriter::Compiler compiler;
    compiler.Executable = executable;
    compiler.Name = "Compiler_" + language;
    file.Write(compiler);
  }
}

void cmGlobalFastbuildGenerator::GenerateBffTargetSection(
  cmFastbuildFileWriter& file, cmMakefile* makefile,
  const std::set<cmGeneratorTarget*>& targets) const
{
  file.WriteSingleLineComment("Targets");

  auto fastbuildTargets = CreateFastbuildTargets(makefile, targets);

  // TODO: Sort
  // TODO: Resolve dependencies

  // Write all targets
  for (const auto& target : fastbuildTargets) {
    file.WriteSingleLineComment("Target " + target.Name);

    // Write pre-build events
    for (const auto& element : target.PreBuildEvents) {
      file.Write(element);
    }

    // Write object lists
    for (const auto& ol : target.ObjectLists) {
      file.Write(ol);
    }

	// Write pre-link events
    for (const auto& element : target.PreLinkEvents) {
      file.Write(element);
    }

	// Write library
    if (target.HasLibrary) {
      file.Write(target.Library);
    }

    // Write post-build events
    for (const auto& element : target.PostBuildEvents) {
      file.Write(element);
    }
  }


 // // TODO: Should sort
 // for (const auto& target : targets) {
 //   // Skip interfaces
 //   if (target->GetType() == cmStateEnums::EXECUTABLE ||
 //       target->GetType() == cmStateEnums::SHARED_LIBRARY ||
 //       target->GetType() == cmStateEnums::STATIC_LIBRARY ||
 //       target->GetType() == cmStateEnums::MODULE_LIBRARY ||
 //       target->GetType() == cmStateEnums::OBJECT_LIBRARY ||
 //       target->GetType() == cmStateEnums::UTILITY ||
 //       target->GetType() == cmStateEnums::GLOBAL_TARGET) {
 //     cmFastbuildTargetGenerator{ file, target }.Generate();
	//}
 // }
}

std::set<cmGeneratorTarget*>
cmGlobalFastbuildGenerator::DetectTargetGenerators() const
{
  std::set<cmGeneratorTarget*> targets;

  // Loop over each target in each generator in each project
  for (const auto& project : GetProjectMap()) {
    const auto& localGenerators = project.second;
    auto root = localGenerators[0];

    for (const auto& lg : localGenerators) {
      // Skip excluded generators
      if (IsExcluded(root, lg))
        continue;

      for (const auto& target : lg->GetGeneratorTargets()) {
        if (IsRootOnlyTarget(target) &&
            lg->GetMakefile() != root->GetMakefile()) {
          continue;
        }

        targets.insert(target);
      }
    }
  }

  return targets;
}

std::set<std::string> fastbuild::detail::DetectTargetLanguages(
  const std::set<cmGeneratorTarget*>& targets)
{
  std::set<std::string> languages;

  for (auto target : targets) {
    // Skip non-code targets
    auto type = target->GetType();
    if (type == cmStateEnums::INTERFACE_LIBRARY ||
        type == cmStateEnums::UTILITY || type == cmStateEnums::GLOBAL_TARGET)
      continue;

    // Loop through all configs
    std::vector<std::string> configs;
    target->Makefile->GetConfigurations(configs, false);
    for (const auto& config : configs) {
      // Loop through all source objects
      std::vector<const cmSourceFile*> sources;
      target->GetObjectSources(sources, config);
      for (const auto& source : sources) {
        auto language = source->GetLanguage();
        if (!language.empty())
          languages.insert(language);
      }
    }
  }

  return languages;
}
