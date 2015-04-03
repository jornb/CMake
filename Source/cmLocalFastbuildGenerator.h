/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalFastbuildGenerator_h
#define cmLocalFastbuildGenerator_h

#include "cmLocalGenerator.h"

#include <cmsys/auto_ptr.hxx>

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
class cmLocalFastbuildGenerator : public cmLocalGenerator
{
public:
  cmLocalFastbuildGenerator();
  virtual ~cmLocalFastbuildGenerator();

  virtual void Generate();

  void ExpandRuleVariables(std::string& s, const RuleVariables& replaceValues);
  virtual std::string ConvertToLinkReference(std::string const& lib,
                                             OutputFormat format);
  virtual void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt);
  virtual std::string GetTargetDirectory(cmTarget const& target) const;
  virtual void AppendFlagEscape(std::string& flags,
                                const std::string& rawFlag);
};

#endif
