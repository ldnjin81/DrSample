using UnrealBuildTool;
public class DrSampleEditorTarget : TargetRules
{
 public DrSampleEditorTarget(TargetInfo Target) : base(Target)
 {
  Type = TargetType.Editor;
  DefaultBuildSettings = BuildSettingsVersion.V7;
  IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
  ExtraModuleNames.Add("DrSample");
 }
}
