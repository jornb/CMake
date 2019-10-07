/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalFastbuildGenerator.h"

#include "cmComputeLinkInformation.h"
#include "cmCustomCommandGenerator.h"
#include "cmGlobalGeneratorFactory.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmState.h"
#include "cmake.h"

#include "cmDocumentationEntry.h"
#include "cmFastbuildLinkLineComputer.h"
#include "cmFastbuildTargetGenerator.h"

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
}

void cmGlobalFastbuildGenerator::Generate()
{
  cmGlobalGenerator::Generate();

  // TODO
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
