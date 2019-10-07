/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFastbuildLinkLineComputer.h"

#include "cmGlobalFastBuildGenerator.h"

cmFastBuildLinkLineComputer::cmFastBuildLinkLineComputer(
  cmOutputConverter* outputConverter, cmStateDirectory stateDir,
  const cmGlobalFastbuildGenerator* gg)
  : cmLinkLineComputer(outputConverter, stateDir)
  , GG(gg)
{
}

std::string cmFastBuildLinkLineComputer::ConvertToLinkReference(
  const std::string& input) const
{
  return GG->ConvertToFastbuildPath(input);
}
