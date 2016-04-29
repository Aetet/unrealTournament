// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealTournamentEditorTarget : TargetRules
{
	public UnrealTournamentEditorTarget(TargetInfo Target)
	{
        Type = TargetType.Editor;
        UEBuildConfiguration.bCompileBox2D = false;
	}

    //
    // TargetRules interface.
    //
    public override bool ConfigureToolchain(TargetInfo Target)
    {
        if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
        {
            WindowsPlatform.Compiler = WindowsCompiler.VisualStudio2013;
        }
        return true;
    }
    public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.Add("UnrealTournament");
        OutExtraModuleNames.Add("UnrealTournamentEditor");

        if (UEBuildConfiguration.bCompileMcpOSS == true)
        {
            OutExtraModuleNames.Add("OnlineSubsystemMcp");
        }
        OutExtraModuleNames.Add("OnlineSubsystemNull");
	}
    public override GUBPProjectOptions GUBP_IncludeProjectInPromotedBuild_EditorTypeOnly(UnrealTargetPlatform HostPlatform)
    {
        var Result = new GUBPProjectOptions();
        Result.bIsPromotable = true;
        Result.bSeparateGamePromotion = true;
        Result.bCustomWorkflowForPromotion = true;
        return Result;
    }
}
