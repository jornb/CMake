/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFastbuildTargetGenerator_h
#define cmFastbuildTargetGenerator_h

#include "cmCommonTargetGenerator.h"

class cmFastbuildTargetGenerator : public cmCommonTargetGenerator
{
public:
  cmFastbuildTargetGenerator(cmGeneratorTarget* gt);
  virtual ~cmFastbuildTargetGenerator() = default;

  void Generate();
};

#endif
