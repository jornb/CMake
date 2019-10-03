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

/** \class cmLocalFastbuildGenerator
 * \brief Base class for Visual Studio generators.
 *
 * cmLocalFastbuildGenerator provides functionality common to all
 * Visual Studio generators.
 */
class cmLocalFastbuildGenerator : public cmLocalCommonGenerator
{
public:
  cmLocalFastbuildGenerator(cmGlobalGenerator* gg, cmMakefile* makefile);
  virtual ~cmLocalFastbuildGenerator();

  virtual void Generate();

  cmRulePlaceholderExpander* CreateRulePlaceholderExpander() const override;

  virtual void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt);
  virtual std::string GetTargetDirectory(
    cmGeneratorTarget const* target) const;
  virtual void AppendFlagEscape(std::string& flags,
                                const std::string& rawFlag);
};

#endif
