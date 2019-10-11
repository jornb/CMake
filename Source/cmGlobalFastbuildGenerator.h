/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalFastbuildGenerator_h
#define cmGlobalFastbuildGenerator_h

#include <vector>
#include <set>

#include "cmGeneratedFileStream.h"
#include "cmGlobalCommonGenerator.h"
#include "cmGlobalGenerator.h"
#include "cmLocalCommonGenerator.h"
#include "cmLocalFastbuildGenerator.h"
#include "cmDocumentationEntry.h"
#include "cmFastbuildFileWriter.h"
#include "cmMakefile.h"

class cmGlobalGeneratorFactory;
class cmCustomCommandGenerator;

namespace fastbuild {
  namespace detail {
    std::set<std::string> DetectTargetLanguages(
      const std::vector<cmGeneratorTarget*>& targets);
  }
}

/** \class cmGlobalFastbuildGenerator
 * \brief Class for global fastbuild generator.
 */
class cmGlobalFastbuildGenerator : public cmGlobalCommonGenerator
{
public:
  cmGlobalFastbuildGenerator(cmake* cm);
  virtual ~cmGlobalFastbuildGenerator() = default;

  // Factory methods with support methods
  static cmGlobalGeneratorFactory* NewFactory();
  static bool SupportsToolset() { return false; }
  static bool SupportsPlatform() { return false; }
  static void GetDocumentation(cmDocumentationEntry& entry);
  static std::string GetActualName() { return "Fastbuild"; }

  // Begin cmGlobalGenerator interface
  void AppendDirectoryForConfig(const std::string& prefix,
                                const std::string& config,
                                const std::string& suffix,
                                std::string& dir) override;
  void ComputeTargetObjectDirectory(cmGeneratorTarget* gt) const override;
  cmLinkLineComputer* CreateLinkLineComputer(
    cmOutputConverter* outputConverter,
    cmStateDirectory const& stateDir) const override;
  cmLocalGenerator* CreateLocalGenerator(cmMakefile* makefile) override;
  void EnableLanguage(std::vector<std::string> const& lang, cmMakefile* mf,
                      bool optional) override;
  void Generate() override;
  std::vector<GeneratedMakeCommand> GenerateBuildCommand(
    const std::string& makeProgram, const std::string& projectName,
    const std::string& projectDir, std::vector<std::string> const& targetNames,
    const std::string& config, bool fast, int jobs, bool verbose,
    std::vector<std::string> const& makeOptions) override;
  const char* GetCMakeCFGIntDir() const override;
  std::string GetName() const override { return GetActualName(); }
  bool IsMultiConfig() const override { return true; }
  // End cmGlobalGenerator interface

  // Utility methods
  std::string ConvertToFastbuildPath(const std::string& path) const;

private:
  std::vector<cmGeneratorTarget*> DetectTargetGenerators() const;

  void GenerateBffFile();
  void GenerateBffCompilerSection(
    cmFastbuildFileWriter& file, cmMakefile* makefile,
    const std::vector<cmGeneratorTarget*>& targets) const;
  void GenerateBffTargetSection(
    cmGlobalGenerator& globalGenerator, cmFastbuildFileWriter& file,
    cmMakefile* makefile,
    const std::vector<cmGeneratorTarget*>& targets) const;
};

#endif
