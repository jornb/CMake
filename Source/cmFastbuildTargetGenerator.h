/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFastbuildTargetGenerator_h
#define cmFastbuildTargetGenerator_h

#include "cmCommonTargetGenerator.h"
#include "cmFastbuildFileWriter.h"

class cmFastbuildTargetGenerator : public cmCommonTargetGenerator
{
public:
  cmFastbuildTargetGenerator(cmFastbuildFileWriter& file,
                             cmGeneratorTarget* gt);
  virtual ~cmFastbuildTargetGenerator() = default;

  void Generate();

  // cmCommonTargetGenerator interface
  void AddIncludeFlags(std::string& flags, std::string const& lang) {}

private:
  std::string GetCompileArguments(const std::string& language);

private:
  cmFastbuildFileWriter& File;
};

#endif
