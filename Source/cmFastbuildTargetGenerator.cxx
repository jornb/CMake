/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFastbuildTargetGenerator.h"

#include <stdexcept>

#include "cmGeneratorTarget.h"
#include "cmGlobalFastbuildGenerator.h"
#include "cmRulePlaceholderExpander.h"
#include "cmSourceFile.h"
#include "cmState.h"

cmFastbuildTargetGenerator::cmFastbuildTargetGenerator(
  cmFastbuildFileWriter& file, cmGeneratorTarget* gt)
  : cmCommonTargetGenerator(gt)
  , File(file){}

namespace {
  cmGeneratorTarget::Names GetOutputNames(cmGeneratorTarget * target,
                                          const std::string& config)
  {
    if (target->GetType() == cmStateEnums::EXECUTABLE)
      return target->GetExecutableNames(config);
    else
      return target->GetLibraryNames(config);
  }

  std::string GetCompilePdbPath(cmGeneratorTarget * target,
                                const std::string& config)
  {
    // First try to get the explicit PDB directory. Otherwise, use the support
    // directory.
    auto pdbDir = target->GetCompilePDBPath(config);
    if (pdbDir.empty())
      pdbDir = target->GetSupportDirectory();

    return pdbDir + "/" + GetOutputNames(target, config).PDB;
  }

  std::string GetCompilerFlags(
    cmLocalCommonGenerator * lg, cmGeneratorTarget * gt,
    const std::vector<cmSourceFile*>& sourceFiles, const std::string& language,
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

  std::string GetCompileDefines(
    cmLocalCommonGenerator * lg, cmGeneratorTarget * gt,
    const std::vector<cmSourceFile*>& sourceFiles, const std::string& config,
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

      compileDefinitions = source->GetProperty(
        "COMPILE_DEFINITIONS_" + cmSystemTools::UpperCase(config));
      if (compileDefinitions)
        lg->AppendDefines(defines, compileDefinitions);
    }

    // Add a definition for the configuration name.
    lg->AppendDefines(defines, "CMAKE_INTDIR=\"" + config + "\"");

    std::string definesString;
    lg->JoinDefines(defines, definesString, language);

    return definesString;
  }
}


void cmFastbuildTargetGenerator::Generate()
{
  auto targetName = GeneratorTarget->GetName();
  File.WriteSingleLineComment("Target " + targetName);

  std::vector<std::string> configs;
  Makefile->GetConfigurations(configs, false);

  // Write object file list for each language and each configuration
  for (const auto& config : configs) {
    // Get all languages (e.g. CXX and or C) in target
    auto languages =
      fastbuild::detail::DetectTargetLanguages({ GeneratorTarget });

    // Start a scope for this configuration
    File.PushScope("{");

    // Write config-wide properties
    File.WriteVariable("TargetOutPDB",
                       GetCompilePdbPath(GeneratorTarget, config), true);

    // Create individual object lists for each language.
    // Note: We are guaranteed that each language has at least one source file
    std::vector<std::string> objectListNames;
    for (const auto& language : languages) {
      cmFastbuildFileWriter::ObjectList objectList;

      // Collect all source files of this language
      std::vector<cmSourceFile*> languageSourceFiles;
      std::vector<std::string> sourceFiles;
      for (const auto& sourceFile : GeneratorTarget->GetSourceFiles(config)) {
        if (sourceFile.Value->GetLanguage() == language)
          languageSourceFiles.push_back(sourceFile.Value);
        sourceFiles.push_back(sourceFile.Value->GetLocation().GetFullPath());
      }

      // If we only have one language, there is no need to prefix the alias
      // with the language
      if (languages.size() == 1)
        objectList.Alias = targetName + "_ObjectList_" + config;
      else
        objectList.Alias =
          targetName + "_ObjectList_" + language + "_" + config;
      objectList.Compiler = "Compiler_" + language;
      objectList.CompilerOptions;
      objectList.CompilerOutputPath =
        GeneratorTarget->GetSupportDirectory() + "/" + config;
      objectList.CompilerInputFiles = sourceFiles;

      // Build compile invocation arguments
      objectList.CompilerOptions +=
        GetCompilerFlags(LocalCommonGenerator, GeneratorTarget,
                         languageSourceFiles, language, config);
      objectList.CompilerOptions += " ";
      objectList.CompilerOptions +=
        GetCompileDefines(LocalCommonGenerator, GeneratorTarget,
                          languageSourceFiles, language, config);
      objectList.CompilerOptions += " ";
      objectList.CompilerOptions += GetCompileArguments(language);

      File.Write(objectList);

      objectListNames.push_back(objectList.Alias);
    }

    // Create an alias for compiling all languages
    if (languages.size() > 1) {
      cmFastbuildFileWriter::Alias alias;
      alias.Name = targetName + "_ObjectList_" + config;
      alias.Targets = objectListNames;
      File.Write(alias);
    }

    // End configuration scope
    File.PopScope("}");
  }
}

std::string cmFastbuildTargetGenerator::GetCompileArguments(
  const std::string& language)
{
  const auto& targetName = GeneratorTarget->GetName();
  const auto targetTypeStr =
    cmState::GetTargetTypeName(GeneratorTarget->GetType());
  const auto& manifests = GetManifests();

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
  compileObjectVars.TargetCompilePDB = "$TargetOutPDB$";

  // Get all commands necessary to compile objects
  std::string compileCmdVariable =
    LocalCommonGenerator->GetMakefile()->GetRequiredDefinition(
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
    static_cast<cmLocalFastbuildGenerator*>(LocalCommonGenerator);
  std::unique_ptr<cmRulePlaceholderExpander> rulePlaceholderExpander{
    localFastbuildGenerator->CreateRulePlaceholderExpander()
  };

  // Expand the compile command
  rulePlaceholderExpander->ExpandRuleVariables(
    localFastbuildGenerator, compileCommand, compileObjectVars);

  return compileCommand;
}
