/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalFastbuildGenerator.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmake.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"

#ifdef _WIN32
#include "windows.h"
#endif

#define FASTBUILD_DOLLAR_TAG "FASTBUILD_DOLLAR_TAG"

cmLocalFastbuildGenerator::cmLocalFastbuildGenerator(cmGlobalGenerator* gg,
                                                     cmMakefile* makefile)
  : cmLocalCommonGenerator(gg, makefile,
                           makefile->GetState()->GetBinaryDirectory())
{ }

void cmLocalFastbuildGenerator::AppendFlagEscape(
  std::string& flags, const std::string& rawFlag) const
{
  std::string escapedFlag = this->EscapeForShell(rawFlag);
  // Other make systems will remove the double $ but
  // fastbuild uses ^$ to escape it. So switch to that.
  // cmSystemTools::ReplaceString(escapedFlag, "$$", "^$");
  this->AppendFlags(flags, escapedFlag);
}

void cmLocalFastbuildGenerator::ComputeObjectFilenames(
  std::map<cmSourceFile const*, std::string>& mapping,
  cmGeneratorTarget const* gt)
{
  for (std::map<cmSourceFile const*, std::string>::iterator si =
         mapping.begin();
       si != mapping.end(); ++si) {
    cmSourceFile const* sf = si->first;
    si->second =
      this->GetObjectFileNameWithoutTarget(*sf, gt->ObjectDirectory);
  }
}

cmRulePlaceholderExpander*
cmLocalFastbuildGenerator::CreateRulePlaceholderExpander() const
{
  cmRulePlaceholderExpander* ret =
    new cmRulePlaceholderExpander(this->Compilers, this->VariableMappings,
                                  this->CompilerSysroot, this->LinkerSysroot);
  ret->SetTargetImpLib(FASTBUILD_DOLLAR_TAG
                       "TargetOutputImplib" FASTBUILD_DOLLAR_TAG);
  return ret;
}

void cmLocalFastbuildGenerator::Generate() { }

std::string cmLocalFastbuildGenerator::GetTargetDirectory(
  const cmGeneratorTarget* target) const
{
  std::string dir = "CMakeFiles/";
  dir += target->GetName();
#if defined(__VMS)
  dir += "_dir";
#else
  dir += ".dir";
#endif
  return dir;
}
