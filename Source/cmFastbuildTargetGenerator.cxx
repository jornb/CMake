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


void cmFastbuildTargetGenerator::Generate()
{
  //auto targetName = GeneratorTarget->GetName();
  //File.WriteSingleLineComment("Target " + targetName);

  //std::vector<std::string> configs;
  //Makefile->GetConfigurations(configs, false);

  //// Write object file list for each language and each configuration
  //for (const auto& config : configs) {
  //  // Get all languages (e.g. CXX and or C) in target
  //  auto languages =
  //    fastbuild::detail::DetectTargetLanguages({ GeneratorTarget });

  //  // Start a scope for this configuration
  //  File.PushScope("{");

  //  // Write config-wide properties
  //  File.WriteVariable("TargetOutPDB",
  //                     GetCompilePdbPath(GeneratorTarget, config), true);

  //  // Create individual object lists for each language.
  //  // Note: We are guaranteed that each language has at least one source file
  //  std::vector<std::string> objectListNames;
  //  for (const auto& language : languages) {
  //    cmFastbuildFileWriter::ObjectList objectList;

  //    // Collect all source files of this language
  //    std::vector<cmSourceFile*> languageSourceFiles;
  //    std::vector<std::string> sourceFiles;
  //    for (const auto& sourceFile : GeneratorTarget->GetSourceFiles(config)) {
  //      if (sourceFile.Value->GetLanguage() == language)
  //        languageSourceFiles.push_back(sourceFile.Value);
  //      sourceFiles.push_back(sourceFile.Value->GetLocation().GetFullPath());
  //    }

  //    // If we only have one language, there is no need to prefix the alias
  //    // with the language
  //    if (languages.size() == 1)
  //      objectList.Alias = targetName + "_ObjectList_" + config;
  //    else
  //      objectList.Alias =
  //        targetName + "_ObjectList_" + language + "_" + config;
  //    objectList.Compiler = "Compiler_" + language;
  //    objectList.CompilerOptions;
  //    objectList.CompilerOutputPath =
  //      GeneratorTarget->GetSupportDirectory() + "/" + config;
  //    objectList.CompilerInputFiles = sourceFiles;

  //    // Build compile invocation arguments
  //    objectList.CompilerOptions +=
  //      GetCompilerFlags(LocalCommonGenerator, GeneratorTarget,
  //                       languageSourceFiles, language, config);
  //    objectList.CompilerOptions += " ";
  //    objectList.CompilerOptions +=
  //      GetCompileDefines(LocalCommonGenerator, GeneratorTarget,
  //                        languageSourceFiles, language, config);
  //    objectList.CompilerOptions += " ";
  //    objectList.CompilerOptions += GetCompileArguments(language);

  //    File.Write(objectList);

  //    objectListNames.push_back(objectList.Alias);
  //  }

  //  // Create an alias for compiling all languages
  //  if (languages.size() > 1) {
  //    cmFastbuildFileWriter::Alias alias;
  //    alias.Name = targetName + "_ObjectList_" + config;
  //    alias.Targets = objectListNames;
  //    File.Write(alias);
  //  }

  //  // End configuration scope
  //  File.PopScope("}");
  //}
}
