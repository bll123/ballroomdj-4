' https://stackoverflow.com/questions/346107/creating-a-shortcut-for-a-exe-from-a-batch-file
' thanks to VVS

set objWSHShell = CreateObject("WScript.Shell")
set objFso = CreateObject("Scripting.FileSystemObject")

sShortcut = objWSHShell.ExpandEnvironmentStrings(WScript.Arguments.Item(0))
sTargetPath = objWSHShell.ExpandEnvironmentStrings(WScript.Arguments.Item(1))
sWorkingDirectory = objFso.GetAbsolutePathName(WScript.Arguments.Item(2))
sProfileNum = objWSHShell.ExpandEnvironmentStrings(WScript.Arguments.Item(3))

set objSC = objWSHShell.CreateShortcut(sShortcut)

objSC.TargetPath = """" & sTargetPath & """"
If sProfileNum > 0 Then
  objSC.Arguments = " --profile " & sProfileNum
End If
objSC.WorkingDirectory = sWorkingDirectory

objSC.Save
