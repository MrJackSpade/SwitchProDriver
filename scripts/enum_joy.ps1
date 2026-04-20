$sig = @'
using System;
using System.Runtime.InteropServices;
public class Joy {
  [DllImport("winmm.dll")] public static extern uint joyGetNumDevs();
  [DllImport("winmm.dll")] public static extern uint joyGetDevCapsW(uint id, out JOYCAPSW caps, uint cbsize);
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
  public struct JOYCAPSW {
    public ushort wMid; public ushort wPid;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string szPname;
    public uint wXmin,wXmax,wYmin,wYmax,wZmin,wZmax,wNumButtons,wPeriodMin,wPeriodMax;
    public uint wRmin,wRmax,wUmin,wUmax,wVmin,wVmax,wCaps,wMaxAxes,wNumAxes,wMaxButtons;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string szRegKey;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string szOEMVxD;
  }
}
'@
Add-Type -TypeDefinition $sig

Write-Host ("joyGetNumDevs = {0}" -f [Joy]::joyGetNumDevs())
for ($i = 0; $i -lt 16; $i++) {
    $c = New-Object Joy+JOYCAPSW
    $r = [Joy]::joyGetDevCapsW($i, [ref]$c, 728)
    if ($r -eq 0) {
        Write-Host ("[ID={0}] VID=0x{1:X4} PID=0x{2:X4} Name='{3}' RegKey='{4}'" -f $i, $c.wMid, $c.wPid, $c.szPname, $c.szRegKey)
    }
}
