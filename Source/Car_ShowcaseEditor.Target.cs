using UnrealBuildTool;
using System.Collections.Generic;

public class Car_ShowcaseEditorTarget : TargetRules
{
	public Car_ShowcaseEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		ExtraModuleNames.Add("Car_Showcase");
	}
}
