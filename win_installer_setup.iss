[Setup]
AppName=MCFunc Compiler
AppVerName=MCFunc Compiler
DefaultDirName={commonpf}\MCFunc Compiler
DefaultGroupName=MCFunc Compiler
OutputDir=.
OutputBaseFilename=mcfunc_installer
PrivilegesRequired=admin

[Files]
Source: ".\build\mcfunc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: ".\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: ".\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
; add app directory to the PATH
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: string; ValueName: "Path"; ValueData: "{olddata};{app}"; Flags: uninsdeletevalue

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    MsgBox('You may need to restart your computer before the command works everywhere.', mbInformation, MB_OK);
  end;
end;
