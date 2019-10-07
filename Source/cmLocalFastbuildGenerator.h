/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalFastbuildGenerator_h
#define cmLocalFastbuildGenerator_h

#include "cmLocalCommonGenerator.h"
#include "cmRulePlaceholderExpander.h"

class cmSourceFile;
class cmSourceGroup;
class cmCustomCommand;
class cmCustomCommandGenerator;

class cmLocalFastbuildGenerator : public cmLocalCommonGenerator
{
public:
  cmLocalFastbuildGenerator(cmGlobalGenerator* gg, cmMakefile* makefile);
  virtual ~cmLocalFastbuildGenerator() = default;

  // cmLocalGenerator interface
  void AppendFlagEscape(std::string& flags,
                        const std::string& rawFlag) const override;
  void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt) override;
  cmRulePlaceholderExpander* CreateRulePlaceholderExpander() const override;
  void Generate() override;
  std::string GetTargetDirectory(
    cmGeneratorTarget const* target) const override;
};

#endif
