@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "TS_SHOT_FILE=%~f0"
set "TS_SHOT_ARGS=%*"
powershell.exe -NoLogo -NoProfile -Command "$file=[IO.File]::ReadAllText($env:TS_SHOT_FILE); . ([scriptblock]::Create($file.Substring($file.IndexOf([char]10+'# PowerShell')+1)))"
set "ts_result=%errorlevel%"
if not "%ts_result%"=="0" if not defined TS_SHOT_WORKER if "%ts_shot_args:--no-pause=%"=="%ts_shot_args%" pause
exit /b %ts_result%

# PowerShell
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = Split-Path -LiteralPath $env:TS_SHOT_FILE
$tools = Join-Path $root 'tools\screenshots'
$demo = Join-Path $tools 'demo.sqlite3'
$utf8 = New-Object Text.UTF8Encoding $false
$nl = [Environment]::NewLine
$compiler = 'msys2'
$action = 'capture'
foreach ($argument in ($env:TS_SHOT_ARGS -split '\s+' | Where-Object { $_ })) {
    switch ($argument) {
        'WACUP' {}
        'msys2' {$compiler = 'msys2'}
        'vs' {$compiler = 'vs'}
        'refresh-db' {$action = 'refresh'}
        'rebuild-db' {$action = 'refresh'}
        '--no-pause' {}
        default { Write-Error 'Usage: regenerate-screenshots.bat [WACUP] [msys2|vs] [rebuild-db] [--no-pause]' }
    }
}
$build = Join-Path $root $(if ($compiler -eq 'vs') {'build-screenshots-vs-v143'} else {'build-screenshots-mingw32'})
$imposter = Join-Path $build 'imposter-navidrome.exe'
$wpf=Join-Path ([Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory()) 'WPF'
$uiAssemblies=@('System.Drawing','Accessibility')+@('UIAutomationClient.dll','UIAutomationTypes.dll','WindowsBase.dll' | ForEach-Object {Join-Path $wpf $_})
Add-Type -ReferencedAssemblies $uiAssemblies -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Automation;
using Accessibility;

// Win32 interop for regenerate-screenshots.bat. Each window operation checks
// both the launched process and its desktop before touching an HWND.
public sealed class CaptureDesktop : IDisposable
{
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    static extern IntPtr CreateDesktop(string name, IntPtr device, IntPtr mode, uint flags, uint access, IntPtr security);
    [DllImport("user32.dll")] static extern bool CloseDesktop(IntPtr desktop);
    [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr CreateJobObject(IntPtr attributes, string name);
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool SetInformationJobObject(IntPtr job, int type, ref JobLimits data, int bytes);
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
    [DllImport("kernel32.dll")] static extern bool TerminateJobObject(IntPtr job, uint code);
    [DllImport("kernel32.dll")] static extern bool TerminateProcess(IntPtr process, uint code);
    [DllImport("kernel32.dll")] static extern uint ResumeThread(IntPtr thread);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    static extern bool CreateProcess(string file, StringBuilder command, IntPtr pa, IntPtr ta, bool inherit,
        uint flags, IntPtr env, string cwd, ref Startup startup, out ProcessInfo process);

    [StructLayout(LayoutKind.Sequential)] struct BasicLimits {
        public long perProcess, perJob; public uint flags; public UIntPtr min, max; public uint active;
        public UIntPtr affinity; public uint priority, scheduling;
    }
    [StructLayout(LayoutKind.Sequential)] struct JobLimits {
        public BasicLimits basic; public ulong rOps,wOps,oOps,rBytes,wBytes,oBytes;
        public UIntPtr processMem, jobMem, peakProcess, peakJob;
    }
    [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct Startup {
        public int size; public string reserved, desktop, title; public int x,y,w,h,xChars,yChars,fill,flags;
        public short show, reservedSize; public IntPtr reservedPtr, stdin, stdout, stderr;
    }
    [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr process, thread; public int pid, tid; }
    IntPtr desktop, job;
    public string Name { get; private set; }
    public CaptureDesktop(string name) {
        Name=name;
        desktop=CreateDesktop(name,IntPtr.Zero,IntPtr.Zero,0,0x1ff,IntPtr.Zero);
        if (desktop==IntPtr.Zero) throw new Win32Exception();
        try {
            job=CreateJobObject(IntPtr.Zero,null);
            var limits=new JobLimits();
            limits.basic.flags=0x2000; // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            if (job==IntPtr.Zero || !SetInformationJobObject(job,9,ref limits,Marshal.SizeOf(limits))) throw new Win32Exception();
        } catch { Dispose(); throw; }
    }
    public Process Start(string file,string command,string cwd) {
        var startup=new Startup();
        startup.size=Marshal.SizeOf(startup);
        startup.desktop=@"WinSta0\"+Name;
        ProcessInfo child;
        if (!CreateProcess(file,new StringBuilder(command),IntPtr.Zero,IntPtr.Zero,false,0x08000004,
                           IntPtr.Zero,cwd,ref startup,out child)) throw new Win32Exception();
        try {
            if (!AssignProcessToJobObject(job,child.process)) throw new Win32Exception();
            var managed=Process.GetProcessById(child.pid);
            if (ResumeThread(child.thread)==UInt32.MaxValue) throw new Win32Exception();
            return managed;
        } catch { TerminateProcess(child.process,1); throw; }
        finally { CloseHandle(child.thread); CloseHandle(child.process); }
    }
    public void Dispose() {
        if (job!=IntPtr.Zero) { TerminateJobObject(job,1); CloseHandle(job); job=IntPtr.Zero; }
        if (desktop!=IntPtr.Zero) { CloseDesktop(desktop); desktop=IntPtr.Zero; }
    }
}

public sealed class CaptureWindows
{
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] static extern IntPtr GetThreadDesktop(uint thread);
    [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode)] static extern int GetPrivateProfileString(string section,string key,string fallback,StringBuilder value,int size,string path);
    [DllImport("user32.dll",SetLastError=true)] static extern bool AttachThreadInput(uint from,uint to,bool attach);
    [DllImport("user32.dll")] static extern IntPtr SetActiveWindow(IntPtr window);
    [DllImport("user32.dll")] static extern IntPtr SetFocus(IntPtr window);
    [DllImport("user32.dll")] static extern IntPtr GetActiveWindow();
    [DllImport("user32.dll")] static extern IntPtr GetFocus();
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern bool GetUserObjectInformation(IntPtr handle,int index,StringBuilder data,int bytes,out int needed);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window,out int process);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr window,StringBuilder text,int count);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window,StringBuilder text,int count);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr window,int id);
    [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr window,uint flag);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr window,out Bounds bounds);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr window,out Bounds bounds);
    [DllImport("user32.dll")] static extern int MapWindowPoints(IntPtr from,IntPtr to,ref Bounds bounds,uint count);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr window,IntPtr after,int x,int y,int w,int h,int flags);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr window,int show);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr SendMessageTimeout(IntPtr window,uint msg,IntPtr w,IntPtr l,uint flags,uint ms,out IntPtr result);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr SendMessageTimeout(IntPtr window,uint msg,IntPtr w,string text,uint flags,uint ms,out IntPtr result);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool PostMessage(IntPtr window,uint msg,IntPtr w,IntPtr l);
    [StructLayout(LayoutKind.Sequential)] struct CopyData {public IntPtr command; public int size; public IntPtr data;}
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
    static extern IntPtr SendMessageTimeout(IntPtr window,uint msg,IntPtr sender,ref CopyData data,uint flags,uint ms,out IntPtr result);
    [DllImport("user32.dll",SetLastError=true)] static extern bool PrintWindow(IntPtr window,IntPtr dc,uint flags);
    [DllImport("user32.dll")] static extern int GetWindowRgn(IntPtr window,IntPtr region);
    [DllImport("gdi32.dll")] static extern IntPtr CreateRectRgn(int l,int t,int r,int b);
    [DllImport("gdi32.dll")] static extern bool DeleteObject(IntPtr obj);
    delegate bool EnumProc(IntPtr window,IntPtr param);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc visit,IntPtr arg);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr parent,EnumProc visit,IntPtr arg);
    [DllImport("oleacc.dll")] static extern int AccessibleObjectFromWindow(IntPtr window,uint id,ref Guid iid,[MarshalAs(UnmanagedType.Interface)] out IAccessible obj);
    [StructLayout(LayoutKind.Sequential)] struct Bounds {public int left,top,right,bottom;}
    readonly int pid;
    readonly string desktop;
    static string DesktopName(uint thread) {
        var name=new StringBuilder(256); int needed;
        if (!GetUserObjectInformation(GetThreadDesktop(thread),2,name,name.Capacity*2,out needed)) throw new Win32Exception();
        return name.ToString();
    }
    public CaptureWindows(int pid,string desktop) {
        this.pid=pid; this.desktop=desktop;
        if (!desktop.StartsWith("TangerineShots-",StringComparison.Ordinal) || DesktopName(GetCurrentThreadId())!=desktop)
            throw new InvalidOperationException("The capture worker must run on its own desktop.");
    }
    bool Owned(IntPtr window) {
        int owner;
        uint thread=GetWindowThreadProcessId(window,out owner);
        return thread!=0 && owner==pid && DesktopName(thread)==desktop;
    }
    void Check(IntPtr window) {
        if (!Owned(window)) throw new InvalidOperationException("The window is outside this WACUP session.");
    }
    public IntPtr[] Windows(IntPtr parent) {
        var all=new List<IntPtr>();
        EnumProc collect=delegate(IntPtr h,IntPtr unused){if(Owned(h)) all.Add(h); return true;};
        if(parent==IntPtr.Zero) EnumWindows(collect,IntPtr.Zero);
        else if (Owned(parent)) EnumChildWindows(parent,collect,IntPtr.Zero);
        return all.ToArray();
    }
    public string Class(IntPtr h) { if(!Owned(h)) return ""; var s=new StringBuilder(128); GetClassName(h,s,s.Capacity); return s.ToString(); }
    public string Text(IntPtr h) {
        if(!Owned(h)) return ""; var s=new StringBuilder(2048); GetWindowText(h,s,s.Capacity); return s.ToString();
    }
    public IntPtr Item(IntPtr h,int id) { return Owned(h)?GetDlgItem(h,id):IntPtr.Zero; }
    public IntPtr Root(IntPtr h) { Check(h); return GetAncestor(h,2); }
    public Rectangle Rect(IntPtr h) { Check(h); Bounds b; GetWindowRect(h,out b); return Rectangle.FromLTRB(b.left,b.top,b.right,b.bottom); }
    public void Place(IntPtr h,int x,int y,int w,int height) {
        Check(h);
        if (!SetWindowPos(h,IntPtr.Zero,x,y,w,height,0x14)) throw new Win32Exception();
    }
    public void Show(IntPtr h,bool visible) {Check(h);ShowWindow(h,visible?4:0);}
    public static string ProfileValue(string file,string section,string key) {
        var value=new StringBuilder(256);
        GetPrivateProfileString(section,key,"",value,value.Capacity,file);
        return value.ToString();
    }
    public void ActivateDialog(IntPtr dialog,IntPtr field) {
        Check(dialog);Check(field);
        int owner;
        uint from=GetCurrentThreadId(),to=GetWindowThreadProcessId(dialog,out owner);
        if(!AttachThreadInput(from,to,true)) throw new Win32Exception();
        try {
            SetWindowPos(dialog,IntPtr.Zero,0,0,0,0,3);
            SetActiveWindow(dialog); SetFocus(field);
            if(GetActiveWindow()!=dialog || GetFocus()!=field) throw new InvalidOperationException("Could not activate the private WACUP dialog.");
            Send(dialog,0x86,1,0); // WM_NCACTIVATE paints the active nonclient title.
        } finally {AttachThreadInput(from,to,false);}
    }
    public long Send(IntPtr h,uint msg,long w,long l) {
        Check(h); IntPtr result;
        if (SendMessageTimeout(h,msg,(IntPtr)w,(IntPtr)l,0x20,10000,out result)==IntPtr.Zero) {
            int error=Marshal.GetLastWin32Error();
            throw new Win32Exception(error==0?1460:error);
        }
        return result.ToInt64();
    }
    public void Post(IntPtr h,uint msg,int w,int l) {
        Check(h); if (!PostMessage(h,msg,(IntPtr)w,(IntPtr)l)) throw new Win32Exception();
    }
    public void SetText(IntPtr h,string value) {
        Check(h); IntPtr result;
        if (SendMessageTimeout(h,12,IntPtr.Zero,value,2,5000,out result)==IntPtr.Zero) throw new Win32Exception();
    }
    public void EnqueueFile(IntPtr player,string path) {
        Check(player);
        var message=new CopyData {command=(IntPtr)1100,size=(path.Length+1)*2,data=Marshal.StringToHGlobalUni(path)};
        try {
            IntPtr result;
            if(SendMessageTimeout(player,0x4a,IntPtr.Zero,ref message,2,5000,out result)==IntPtr.Zero) throw new Win32Exception();
        } finally {Marshal.FreeHGlobal(message.data);}
    }
    public string ControlText(IntPtr h) {
        Check(h); IntPtr buf=Marshal.AllocHGlobal(8192);
        try { Send(h,13,4096,buf.ToInt64()); return Marshal.PtrToStringUni(buf); } finally { Marshal.FreeHGlobal(buf); }
    }
    public bool SelectTree(IntPtr tree,string path) {
        Check(tree);
        var guid=typeof(IAccessible).GUID; IAccessible acc;
        if (AccessibleObjectFromWindow(tree,0xfffffffc,ref guid,out acc)!=0) return false;
        try {
            long item=Send(tree,0x110a,0,0);
            foreach(string name in path.Split('/')) {
                while(item!=0 && acc.get_accName((int)Send(tree,0x112b,item,0))!=name) item=Send(tree,0x110a,1,item);
                if(item==0) return false;
                if (name==path.Substring(path.LastIndexOf('/')+1)) {
                    Send(tree,0x110b,9,item);
                    return Send(tree,0x110a,9,0)==item;
                }
                Send(tree,0x1102,2,item);
                item=Send(tree,0x110a,4,item);
            }
            return false;
        } finally {Marshal.ReleaseComObject(acc);}
    }
    public bool SelectListItem(IntPtr list,string title) {
        Check(list);
        var guid=typeof(IAccessible).GUID; IAccessible acc;
        if(AccessibleObjectFromWindow(list,0xfffffffc,ref guid,out acc)!=0) return false;
        try {
            int count=(int)Send(list,0x1004,0,0);
            if(count>100) return false; // the unfiltered catalog is still on screen
            for(int i=0;i<count;i++) {
                if(!String.Equals(acc.get_accName(i+1),title,StringComparison.Ordinal)) continue;
                Send(list,0x100,0x24,0);
                for(int row=0;row<i;row++) Send(list,0x100,0x28,0);
                return (Send(list,0x102c,i,2)&2)!=0;
            }
            return false;
        } catch(ArgumentException) {return false;}
        catch(COMException) {return false;}
        finally {Marshal.ReleaseComObject(acc);}
    }
    public Bitmap Print(IntPtr h) {
        var size=Rect(h).Size;
        if (size.Width<1 || size.Height<1 || size.Width>6000 || size.Height>6000) throw new InvalidOperationException("Invalid window dimensions.");
        var image=new Bitmap(size.Width,size.Height,System.Drawing.Imaging.PixelFormat.Format24bppRgb);
        try {
            using (var g=Graphics.FromImage(image)) {
                var dc=g.GetHdc();
                try { if (!PrintWindow(h,dc,2)) throw new Win32Exception(); }
                finally {g.ReleaseHdc(dc);}
            }
            return image;
        } catch {image.Dispose();throw;}
    }
    public Region Shape(IntPtr h) {
        Check(h); var region=CreateRectRgn(0,0,0,0);
        try {return GetWindowRgn(h,region)>1 ? Region.FromHrgn(region) : new Region(new Rectangle(Point.Empty,Rect(h).Size));}
        finally {DeleteObject(region);}
    }
    public bool SpectrumMoving(IntPtr window) {
        using(var first=Print(window)) {
            System.Threading.Thread.Sleep(130);
            using(var next=Print(window)) {
                int changed=0;
                for(int y=45;y<first.Height-16;y+=3) for(int x=16;x<first.Width-16;x+=3) {
                    if(first.GetPixel(x,y)!=next.GetPixel(x,y) && ++changed>=8) return true;
                }
            }
        }
        return false;
    }
    public string ArtworkStatus { get; private set; }
    public bool BentoArtwork(IntPtr player,string filename) {
        using(var expected=new Bitmap(filename))
        using(var frame=Print(player)) {
            foreach(var area in new[]{new Rectangle(454,53,170,170),new Rectangle(825,377,312,312)}) {
                int error=0, samples=0;
                for(int y=1;y<16;y++) for(int x=1;x<16;x++) {
                    var a=expected.GetPixel(x*expected.Width/16,y*expected.Height/16);
                    var b=frame.GetPixel(area.Left+x*area.Width/16,area.Top+y*area.Height/16);
                    error+=Math.Abs(a.R-b.R)+Math.Abs(a.G-b.G)+Math.Abs(a.B-b.B);
                    samples+=3;
                }
                if(error>samples*32) {
                    ArtworkStatus="Artwork at "+area+" differs from the track cover by "+(error/samples)+" levels per channel.";
                    return false;
                }
            }
            return true;
        }
    }
    // The Classic Modern skin defines a 62px thumb on a track starting at x=19
    // with a 17px right margin. Locate the light handle on the dark groove in PrintWindow.
    public double ClassicModernSeekFraction(IntPtr player) {
        using (var frame=Print(player)) {
            int width=frame.Width;
            var brightness=new int[width];
            int minimum=765, maximum=0;
            for(int x=20;x<width-18;x++) {
                for(int y=146;y<=158;y+=2) {
                    var pixel=frame.GetPixel(x,y);
                    brightness[x]=Math.Max(brightness[x],pixel.R+pixel.G+pixel.B);
                }
                minimum=Math.Min(minimum,brightness[x]);
                maximum=Math.Max(maximum,brightness[x]);
            }
            int pixels=0,sum=0,threshold=minimum+(maximum-minimum)*2/3;
            for(int x=20;x<width-18;x++) if(brightness[x]>threshold) {pixels++;sum+=x;}
            return pixels<12 || pixels>75 ? -1 : (sum/(double)pixels-50)/(width-98);
        }
    }
    public string FitPopulatedColumns(IntPtr list) {
        Check(list);
        IntPtr header=(IntPtr)Send(list,0x101f,0,0);
        if(header==IntPtr.Zero || !IsWindowVisible(header)) return "";
        int rows=(int)Send(list,0x1004,0,0), columns=(int)Send(header,0x1200,0,0);
        if(rows==0) {Show(header,false);return "empty list";}
        if(columns<=1) return "";
        object pattern;
        var table=AutomationElement.FromHandle(list);
        if(!table.TryGetCurrentPattern(GridPattern.Pattern,out pattern)) throw new InvalidOperationException("The library table did not expose its cells.");
        var grid=(GridPattern)pattern;
        int first=(int)Send(list,0x1027,0,0), last=Math.Min(rows,first+(int)Send(list,0x1028,0,0)+1);
        Bounds bounds;
        if(!GetClientRect(list,out bounds)) throw new Win32Exception();
        int viewportBottom=Rect(list).Bottom;
        var names=new string[columns];
        var minimum=new int[columns];
        var preferred=new int[columns];
        var flex=new int[columns];
        var report=new List<string>();
        int minimumTotal=0, contentTotal=0, flexTotal=0;
        var headers=AutomationElement.FromHandle(header).FindAll(TreeScope.Children,Condition.TrueCondition);
        if(headers.Count!=columns) throw new InvalidOperationException("The library table did not expose all of its headings.");
        using(var g=Graphics.FromHwnd(list)) {
            for(int col=0;col<columns;col++) {
                names[col]=headers[col].Current.Name;
                int widest=0;
                for(int row=first;row<last;row++) {
                    var cell=grid.GetItem(row,col);
                    if(cell==null || cell.Current.BoundingRectangle.Top>=viewportBottom) continue;
                    string value=cell.Current.Name;
                    if(String.IsNullOrWhiteSpace(value) || value.Trim() == "\u2800") continue;
                    widest=Math.Max(widest,(int)Math.Ceiling(g.MeasureString(value,SystemFonts.MessageBoxFont).Width)+12);
                }
                if(widest==0) {report.Add(names[col]+" hidden");continue;}
                minimum[col]=(int)Math.Ceiling(g.MeasureString(names[col],SystemFonts.MessageBoxFont).Width)+18;
                preferred[col]=Math.Max(minimum[col],widest);
                bool compact=Array.IndexOf(new[]{"Cache","Length","Tracks","Albums","Year","Disc","#"},names[col])>=0;
                flex[col]=compact?0:preferred[col];
                minimumTotal+=minimum[col]; contentTotal+=preferred[col]; flexTotal+=flex[col];
            }
        }
        if(contentTotal==0) throw new InvalidOperationException("The nonempty library table returned no visible values.");
        int space=bounds.right-3, weighted=0, assigned=0;
        bool shrink=contentTotal>space;
        int extra=shrink?Math.Max(0,space-minimumTotal):space-contentTotal;
        int weights=shrink?contentTotal-minimumTotal:(flexTotal==0?contentTotal:flexTotal);
        for(int col=0;col<columns;col++) {
            int baseWidth=shrink?minimum[col]:preferred[col];
            weighted+=shrink?preferred[col]-minimum[col]:(flexTotal==0?preferred[col]:flex[col]);
            int share=weights==0?0:(int)((long)extra*weighted/weights);
            int width=baseWidth+share-assigned;
            Send(list,0x101e,col,width);
            if(width>0) report.Add(names[col]+" "+width+"px");
            assigned=share;
        }
        return String.Join(", ",report.ToArray());
    }
    public void FitBackButton(IntPtr form) {
        var back=Item(form,1007);
        if(back==IntPtr.Zero || !IsWindowVisible(back)) return;
        int extra=Math.Max(0,60-Rect(back).Width);
        if(extra==0) return;
        foreach(int id in new int[]{1007,1014,1003}) {
            IntPtr child=Item(form,id); Check(child);
            Bounds box; GetWindowRect(child,out box);
            MapWindowPoints(IntPtr.Zero,form,ref box,2);
            int x=box.left+(id==1007?0:extra);
            int width=box.right-box.left+(id==1007?extra:id==1003?-extra:0);
            Place(child,x,box.top,width,box.bottom-box.top);
        }
    }
    // An owner-data icon list exposes each tile's screen bounds through MSAA.
    // Count painted covers only in the image area above the two caption lines.
    public string CoverStatus { get; private set; }
    public int FullCovers { get; private set; }
    public int CoversAcross { get; private set; }
    public double BottomArtworkFraction { get; private set; }
    public bool TopAlbum(IntPtr list,int index) {
        Check(list);
        var guid=typeof(IAccessible).GUID; IAccessible acc;
        if(AccessibleObjectFromWindow(list,0xfffffffc,ref guid,out acc)!=0) return false;
        try {
            Send(list,0x1013,0,0); // Get the first tile's row origin including any native header.
            int x,top,w,h,y;
            acc.accLocation(out x,out top,out w,out h,1);
            Send(list,0x1013,index,0);
            acc.accLocation(out x,out y,out w,out h,index+1);
            if(y!=top) Send(list,0x1014,0,y-top);
            acc.accLocation(out x,out y,out w,out h,index+1);
            return Math.Abs(y-top)<=2;
        } catch(COMException) { return false; }
        catch(ArgumentException) { return false; }
        finally {Marshal.ReleaseComObject(acc);}
    }
    public bool CoversPainted(IntPtr list,string[] allowedAlbums,int firstIndex) {
        Check(list);
        var guid=typeof(IAccessible).GUID; IAccessible acc;
        if(AccessibleObjectFromWindow(list,0xfffffffc,ref guid,out acc)!=0) return false;
        try {
            var box=Rect(list);
            int checkedTiles=0, bottomRow=Int32.MinValue;
            FullCovers=0; CoversAcross=0; BottomArtworkFraction=0;
            var rowCounts=new Dictionary<int,int>();
            int background=(int)Send(list,0x1000,0,0), ink=(int)Send(list,0x1023,0,0);
            var paper=background==-1 ? SystemColors.Window : ColorTranslator.FromWin32(background);
            var text=ColorTranslator.FromWin32(ink);
            var placeholder=Color.FromArgb((paper.R*7+text.R)/8,(paper.G*7+text.G)/8,(paper.B*7+text.B)/8);
            using(var pixels=Print(list)) {
                int rows=(int)Send(list,0x1004,0,0);
                for(int id=Math.Max(1,firstIndex-11);id<=Math.Min(firstIndex+32,rows);id++) {
                    int x,y,w,h;
                    try {acc.accLocation(out x,out y,out w,out h,id);}
                    catch(ArgumentException) {CoverStatus="No location for cover "+id;return false;}
                    catch(COMException) {CoverStatus="Unavailable cover "+id;return false;}
                    x-=box.X; y-=box.Y;
                    if(y>=box.Height || h<65 || w<50) continue;
                    var photo=Rectangle.Intersect(new Rectangle(0,0,box.Width,box.Height),
                        new Rectangle(x+5,y+5,Math.Max(1,w-10),Math.Max(1,Math.Min(w,h)-12)));
                    if(photo.Width<20 || photo.Height<12) continue;
                    if(allowedAlbums!=null && Array.IndexOf(allowedAlbums,acc.get_accName(id))<0) {
                        CoverStatus="Unexpected visible album: "+acc.get_accName(id);
                        return false;
                    }
                    var colors=new HashSet<int>();
                    for(int py=photo.Top;py<photo.Bottom && colors.Count<16;py+=4)
                        for(int px=photo.Left;px<photo.Right && colors.Count<16;px+=4)
                            colors.Add(pixels.GetPixel(px,py).ToArgb());
                    colors.Remove(paper.ToArgb());
                    colors.Remove(placeholder.ToArgb());
                    if(colors.Count==0) {
                        CoverStatus="Waiting for "+acc.get_accName(id)+" at "+photo+" ("+colors.Count+" colors).";
                        return false;
                    }
                    checkedTiles++;
                    int across; rowCounts.TryGetValue(y,out across);
                    rowCounts[y]=++across;
                    CoversAcross=Math.Max(CoversAcross,across);
                    if(y>bottomRow) {
                        bottomRow=y;
                        BottomArtworkFraction=Math.Min(1,(box.Height-y)/(double)Math.Min(w,h));
                    }
                    if(y>=0 && y+h<=box.Height) FullCovers++;
                }
            }
            return checkedTiles>0;
        } finally {Marshal.ReleaseComObject(acc);}
    }
}
'@
Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text;

// The verified NSIS package names and hashes the auxiliary downloads selected
// by this WACUP release. Read its header without running the installer.
public static class WacupPackage {
    public static string Manifest(string installer,string sevenZip,string directory) {
        byte[] data=File.ReadAllBytes(installer);
        byte[] signature=Encoding.ASCII.GetBytes("NullsoftInst");
        int offset=0;
        for (int i=8;i<data.Length-24;i+=512) {
            bool match=true;
            for (int j=0;j<signature.Length;j++) if(data[i+j]!=signature[j]) {match=false;break;}
            if (match) {offset=i;break;}
        }
        if (offset==0) throw new InvalidDataException("Unrecognized WACUP installer format.");
        int length=BitConverter.ToInt32(data,offset+12);
        if (length<16 || length>32*1024*1024) throw new InvalidDataException("Invalid WACUP package header.");
        string archive=Path.Combine(directory,"installer-header.lzma");
        using (var stream=File.Create(archive)) {
            stream.Write(data,offset+20,5);
            for(int i=0;i<8;i++) stream.WriteByte(255);
            stream.Write(data,offset+25,data.Length-offset-25);
        }
        var info=new ProcessStartInfo(sevenZip,"x -so -tlzma \""+archive+"\"");
        info.UseShellExecute=false; info.CreateNoWindow=true; info.RedirectStandardOutput=true;
        using (var reader=Process.Start(info)) {
            try {
                using(var binary=new BinaryReader(reader.StandardOutput.BaseStream)) {
                    if(binary.ReadInt32()!=length) throw new InvalidDataException("WACUP header length mismatch.");
                    byte[] header=binary.ReadBytes(length);
                    if(header.Length!=length) throw new EndOfStreamException();
                    return Encoding.Unicode.GetString(header);
                }
            } finally {if(!reader.HasExited) reader.Kill(); reader.WaitForExit();}
        }
    }
    // WACUP ships NSIS VPatch deltas for the Winamp modern-skin plug-in and font renderer.
    public static void ApplyPatch(string file,string patch) {
        byte[] original=File.ReadAllBytes(file);
        using (var md5=MD5.Create())
        using (var delta=new BinaryReader(File.OpenRead(patch))) {
            if(delta.ReadUInt32()!=0x54415056 || delta.ReadUInt32()!=0x80000001)
                throw new InvalidDataException("Unrecognized WACUP skin-support patch.");
            int blocks=delta.ReadInt32();
            string before=BitConverter.ToString(delta.ReadBytes(16)), after=BitConverter.ToString(delta.ReadBytes(16));
            string input=BitConverter.ToString(md5.ComputeHash(original));
            if (input==after) return;
            if (input!=before) throw new InvalidDataException("Incorrect source version for "+Path.GetFileName(file));
            delta.ReadInt32();
            using(var result=new MemoryStream()) {
                for(int i=0;i<blocks;i++) {
                    byte op=delta.ReadByte();
                    if(op==255) {delta.ReadInt64();continue;}
                    if(op<1 || op>7 || op==4) throw new InvalidDataException("Invalid VPatch operation.");
                    int width=op&3;
                    int count=width==1?delta.ReadByte():width==2?delta.ReadUInt16():delta.ReadInt32();
                    if(op<4) result.Write(original,delta.ReadInt32(),count);
                    else {var part=delta.ReadBytes(count);result.Write(part,0,part.Length);}
                }
                byte[] output=result.ToArray();
                if(BitConverter.ToString(md5.ComputeHash(output))!=after) throw new InvalidDataException("Skin-support patch checksum mismatch.");
                File.WriteAllBytes(file,output);
            }
        }
    }
}
'@
Add-Type -AssemblyName System.Drawing
function Write-Log($value) {
    if ($env:TS_SHOT_WORKER) { [IO.File]::AppendAllText((Join-Path $run 'progress'), $value + $nl, $utf8) }
    else { Write-Host $value }
}
function Remove-RunDirectory([string]$parent,[string]$name) {
    $path = [IO.Path]::GetFullPath((Join-Path $parent $name))
    if ([IO.Path]::GetDirectoryName($path) -ne [IO.Path]::GetFullPath($parent) -or
        $name -notmatch '^TangerineShots-[0-9a-f]{32}$') { throw 'Invalid capture workspace.' }
    if (Test-Path -LiteralPath $path) {
        $item = Get-Item -Force -LiteralPath $path
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Unexpected link in $path" }
        for ($attempt=0; ; ++$attempt) {
            try { Remove-Item -LiteralPath $path -Force -Recurse; break }
            catch { if ($attempt -ge 8) { throw }; Start-Sleep -Milliseconds 350 }
        }
    }
}
function Get-LatestInstaller {
    $page = [Uri]'https://getwacup.com/preview/'
    $html = (Invoke-WebRequest -UseBasicParsing $page).Content
    $link = [regex]::Match($html,'(?i)href="([^"]*WACUP_Preview_v[0-9_]+_x86\.exe)"').Groups[1].Value
    if (!$link) { throw 'The current x86 installer link is missing from the WACUP preview page.' }
    $uri = [Uri]::new($page,$link)
    if ($uri.Scheme -ne 'https' -or $uri.Host -ne $page.Host) { throw 'Unexpected WACUP download origin.' }
    $hash = [regex]::Match(($html -replace '(?i)<wbr\s*/?>',''), ('(?i)href="[^"]*'+[regex]::Escape($uri.Segments[-1])+'\.sha256">\s*([a-f0-9]{64})')).Groups[1].Value
    if (!$hash) { throw 'WACUP did not publish a checksum for the selected installer.' }
    $installer = Join-Path $run $uri.Segments[-1]
    $cached = Join-Path $env:APPDATA ('WACUP\Updates\'+$uri.Segments[-1])
    if ((Test-Path -LiteralPath $cached) -and (Get-FileHash -LiteralPath $cached).Hash -eq $hash) {
        Copy-Item -LiteralPath $cached -Destination $installer
    } else { Invoke-WebRequest -UseBasicParsing $uri -OutFile $installer }
    if ((Get-FileHash -LiteralPath $installer).Hash -ne $hash) { throw 'WACUP installer checksum mismatch.' }
    Write-Log ("Using " + $uri.Segments[-1])
    return $installer
}
function Start-PrivateProcess($file,$arguments,$directory,[bool]$stdin=$false,[bool]$stdout=$false) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $file
    $info.Arguments = $arguments
    $info.WorkingDirectory = $directory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $stdin
    $info.RedirectStandardOutput = $stdout
    return [Diagnostics.Process]::Start($info)
}
function Write-Login($process,$username,$password) {
    $stream = $process.StandardInput.BaseStream
    $bytes = $utf8.GetBytes($username + $nl + $password + $nl)
    try { $stream.Write($bytes,0,$bytes.Length); $stream.Flush() }
    finally { [Array]::Clear($bytes,0,$bytes.Length); $process.StandardInput.Close() }
}
function Invoke-Refresh {
    $pipe=[IO.Pipes.NamedPipeClientStream]::new('.',$env:TS_SHOT_WORKER,[IO.Pipes.PipeDirection]::In)
    try {
        $pipe.Connect(20000)
        $reader=[IO.StreamReader]::new($pipe,$utf8)
        try {$password=$reader.ReadLine()} finally {$reader.Dispose()}
    } finally {$pipe.Dispose()}
    Build-Tools
    $export=Start-PrivateProcess $imposter ('refresh "'+(Join-Path $run 'demo.sqlite3')+'"') $run $true $true
    try {
        Write-Login $export 'okeeblow' $password
        $password=$null
        Write-Log $export.StandardOutput.ReadToEnd().TrimEnd()
        $export.WaitForExit()
        if ($export.ExitCode) {throw 'Could not refresh the demo database. Check the Navidrome login and network connection.'}
    } finally {
        if (!$export.HasExited) {$export.Kill(); $export.WaitForExit()}
        $export.Dispose()
    }
}
function Build-Tools {
    $env:WACUP_PLUGINS_DIR = Join-Path $run 'WACUP\Plugins'
    $log = Join-Path $run 'build.log'
    $entry=Join-Path $root 'build-and-install.bat'
    $builder=Start-PrivateProcess $env:ComSpec ('/d /c ""'+$entry+'" '+$compiler+' --screenshots --no-pause >"'+$log+'" 2>&1"') $root
    try {
        $builder.WaitForExit()
        if ($builder.ExitCode) {throw ([IO.File]::ReadAllText($log))}
    } finally {$builder.Dispose()}
}
function Install-PrivateWacup {
    $app = Join-Path $run 'WACUP'
    $installer = Get-LatestInstaller
    $sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue).Source
    if (!$sevenZip) { $sevenZip = Join-Path $env:ProgramFiles '7-Zip\7z.exe' }
    if (!(Test-Path -LiteralPath $sevenZip)) { throw 'Install 7-Zip to unpack the WACUP x86 distribution.' }
    $manifest = [WacupPackage]::Manifest($installer,$sevenZip,$run)
    function Expand-Package($archive,$to,$entries) {
        & $sevenZip x -y "-o$to" $archive @entries | Out-Null
        if ($LASTEXITCODE) {throw "Could not unpack $archive."}
    }
    function Get-Component($cacheName,$url) {
        $cached = Join-Path $env:LOCALAPPDATA ("WACUP\"+$cacheName)
        if ((Test-Path -LiteralPath $cached) -and $manifest.Contains((Get-FileHash -LiteralPath $cached).Hash)) {return $cached}
        $file = Join-Path $run $cacheName
        Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $file
        if (!$manifest.Contains((Get-FileHash -LiteralPath $file).Hash)) {throw "WACUP component checksum mismatch for $cacheName."}
        return $file
    }
    $skinUrl = [regex]::Match($manifest,'https://getwacup\.com/files/wacup_modern_skins_[0-9]+\.7z').Value
    if (!$skinUrl) {throw 'The WACUP installer did not specify its Modern skin archive.'}
    $skins = Get-Component 'wacup_default_modern_skins.7z' $skinUrl
    $redux = Get-Component 'winamp_5666_full_redux_files.7z' 'https://getwacup.com/get_redux.php'
    Expand-Package $installer $app @('*.dll','wacup.exe','winamp.exe','Microsoft.VC*.CRT','System',
        'Plugins\in_wave.dll','Plugins\in_wav.dll','Plugins\gen_lyrics.dll','Plugins\ml_ll.dll','Plugins\ml_playlists.dll',
        'Plugins\ml_bookmarks.dll','Plugins\ml_history.dll','Plugins\gen_ff.patch')
    Expand-Package $redux $app @('Plugins\gen_ff.dll','Plugins\freeform','Microsoft.VC90.CRT')
    Expand-Package $skins (Join-Path $app 'Skins') @('Winamp Classic Modern','Big Bento Modern')
    Expand-Package $skins (Join-Path $app 'Plugins') @('freeform')
    $eqProfile=Join-Path $run 'profile'
    & $sevenZip e -y "-o$eqProfile" $installer '*\winamp.q1' | Out-Null
    if ($LASTEXITCODE -or !(Test-Path -LiteralPath (Join-Path $eqProfile 'winamp.q1'))) {throw 'Missing WACUP factory EQ presets.'}
    $presetDir=Join-Path $eqProfile 'Plugins\vis_classic'
    & $sevenZip e -y "-o$presetDir" $installer '*\Plugins\vis_classic\*.ini' | Out-Null
    if ($LASTEXITCODE) {throw 'Could not extract WACUP analyzer presets.'}
    & $sevenZip e -y ('-o'+(Join-Path $app 'Plugins')) $installer '*\vis_classic.dll' | Out-Null
    if ($LASTEXITCODE) {throw 'Could not extract the Classic Spectrum Analyzer.'}
    [WacupPackage]::ApplyPatch((Join-Path $app 'Plugins\gen_ff.dll'),(Join-Path $app 'Plugins\gen_ff.patch'))
    $font = Join-Path $app 'Plugins\freeform\wacs\freetype\freetype'
    [WacupPackage]::ApplyPatch(($font+'.wac'),($font+'.patch'))
    foreach ($folder in 'Roaming','Local','Temp') { [void][IO.Directory]::CreateDirectory((Join-Path $run $folder)) }
    foreach ($name in 'wacup.exe','Skins\Winamp Classic Modern\skin.xml','Skins\Big Bento Modern\skin.xml','Plugins\gen_ff.dll') {
        if (!(Test-Path -LiteralPath (Join-Path $app $name))) { throw "Portable WACUP is missing $name." }
    }
    $frame = Join-Path $app 'Skins\Winamp Classic Modern\xml\standardframe.xml'
    $xml = [IO.File]::ReadAllText($frame)
    $group = '<groupdef\b(?=[^>]*\bid="wasabi\.standardframe\.nostatusbar")[^>]*>'
    $matchesFound = [regex]::Matches($xml,$group)
    if ($matchesFound.Count -ne 1) { throw 'Could not identify the Classic Modern window-frame group.' }
    $tag = $matchesFound[0].Value
    if ($tag -notmatch '\brectrgn="1"') {
        $fixed = if ($tag -match '\brectrgn="[^"]*"') { $tag -replace '\brectrgn="[^"]*"','rectrgn="1"' }
                 else { $tag.Replace('id="wasabi.standardframe.nostatusbar"','id="wasabi.standardframe.nostatusbar" rectrgn="1"') }
        [IO.File]::WriteAllText($frame,$xml.Replace($tag,$fixed),$utf8)
    }
    if ([regex]::Match([IO.File]::ReadAllText($frame),$group).Value -notmatch '\brectrgn="1"') { throw 'Classic Modern frame patch failed.' }
}
function Wait-For([scriptblock]$check,[string]$description,[int]$timeoutSeconds=25) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    do {
        if ($player -and $player.HasExited) { throw "WACUP exited while waiting for $description." }
        try {
            $value = & $check
            if ($value) { return $value }
        } catch [ComponentModel.Win32Exception] {
            # A catalog update can temporarily keep WACUP from processing a timed window message.
            if ($_.Exception.GetBaseException().NativeErrorCode -notin 0,1400,1460) {throw}
        }
        Start-Sleep -Milliseconds 125
    } while ($timer.Elapsed.TotalSeconds -lt $timeoutSeconds)
    throw "Timed out waiting for $description."
}
function Get-Top($class,$caption='') {
    foreach ($window in $ui.Windows([IntPtr]::Zero)) {
        if ($ui.Class($window) -eq $class -and (!$caption -or $ui.Text($window) -eq $caption)) { return $window }
    }
}
function Find-Form($id) {
    $companion = switch ($id) {1001 {1017} 1021 {1034} 1003 {1016} 1060 {1053} 1067 {1068} default {throw "Unknown form control $id."}}
    foreach ($window in $ui.Windows([IntPtr]::Zero)) {
        foreach ($child in @($window)+$ui.Windows($window)) {
            if ($ui.Class($child) -eq '#32770' -and [CaptureWindows]::IsWindowVisible($child) -and
                $ui.Item($child,$companion) -ne [IntPtr]::Zero -and $ui.Item($child,$id) -ne [IntPtr]::Zero -and
                [CaptureWindows]::IsWindowVisible($ui.Item($child,$id))) {return $child}
        }
    }
}
function Select-View($name) {
    $path = if ($name) {'Tangerine Stream/'+$name} else {'Tangerine Stream'}
    [void](Wait-For { $ui.SelectTree($tree,$path) } $path)
}
function Command($form,$id) { $ui.Post($form,0x111,$id,0) }
function Click-First($list) {
    [void](Wait-For {$ui.Send($list,0x1004,0,0) -gt 0} 'first list row')
    [void]$ui.Send($list,0x100,0x24,0)
    [void]$ui.Send($list,0x101,0x24,0)
    if (!$ui.Send($list,0x1032,0,0)) { throw 'WACUP did not select the first row.' }
}
function Wait-Library([bool]$empty=$false) {
    $form = Wait-For { Find-Form 1003 } 'Tangerine Stream library'
    [void](Wait-For {
        $status = $ui.ControlText($ui.Item($form,1013))
        $status -and $status -notmatch 'Loading|Server offline|error|failed|Wrong credentials' -and ($empty -or $ui.Send($ui.Item($form,1006),0x1004,0,0) -gt 0)
    } 'finished library page' 150)
    return $form
}
function Get-Frames {
    foreach ($window in $ui.Windows([IntPtr]::Zero)) {
        if (![CaptureWindows]::IsWindowVisible($window)) {continue}
        $rect = $ui.Rect($window)
        if ($rect.Width -le 2 -or $rect.Height -le 2 -or $rect.X -le -30000) {continue}
        $class = $ui.Class($window)
        if ($class -eq 'BaseWindow_RootWnd' -or $class -eq '#32770' -or $class -eq '#32768' -or
            ($classic -and ($class -like 'Winamp*' -or $class -eq 'Winamp Gen'))) { $window }
    }
}
function Get-FactoryPresets {
    $data=[IO.File]::ReadAllBytes((Join-Path $run 'profile\winamp.q1'))
    $signature="Winamp EQ library file v1.1"+[char]26+'!--'
    if ([Text.Encoding]::ASCII.GetString($data,0,31) -cne $signature -or ($data.Length-31)%268) {throw 'Unrecognized WACUP EQ library.'}
    $presets=@{}
    for ($offset=31;$offset -lt $data.Length;$offset+=268) {
        $label=[Text.Encoding]::ASCII.GetString($data,$offset,257).Split([char]0)[0]
        $presets[$label]=$data[($offset+257)..($offset+267)]
    }
    return $presets
}
function Set-TrackEqualizer($song) {
    $preset=switch -Regex ($song.genre) {
        'reggae|dub(?!step)' {'Reggae';break}
        'ska\b' {'Ska';break}
        'big beat|house|disco|dance' {'Dance';break}
        'techno|breakcore|drum.*bass|trance|dubstep' {'Techno';break}
        'jazz|big band|swing' {'Live';break}
        'hip.?hop|trip.?hop|rap|slice of life' {'Party';break}
        'rock|metal|punk|shoegaze|^mod$|industrial' {'Rock';break}
        'video game|funk' {'Club';break}
        'classical|orchestral' {'Classical';break}
        'soundtrack' {if ($song.artist -eq 'Jerry Martin') {'Live'} else {'Classical'};break}
        'folk|acoustic|ambient' {'Soft';break}
        'pop|indietronica' {'Pop';break}
        default {'Flat'}
    }
    $curve=$factoryPresets[$preset]
    if ($curve.Count -ne 11) {throw "WACUP did not supply its $preset preset."}
    # IPC_SETEQDATA accepts a tagged band index. The last two slots enable EQ and disable song autoload.
    $values=@($curve)+@(1,0)
    for ($band=0;$band -lt $values.Count;++$band) {
        [void]$ui.Send($main,0x400,(0xdb000000L -bor ($band -shl 16) -bor $values[$band]),128)
    }
    for ($band=0;$band -lt $values.Count;++$band) {
        if ($ui.Send($main,0x400,$band,127) -ne $values[$band]) {throw "WACUP did not apply $preset at EQ band $band."}
    }
    return $preset
}
function Set-AnalyzerPreset($child,$preset) {
    $settings=Join-Path $profile 'Plugins\vis_classic.ini'
    for ($i=0;$i -le 28;++$i) {
        $previous=[CaptureWindows]::ProfileValue($settings,'Classic Analyzer','Profile')
        if ($previous -eq $preset) {return}
        $ui.Post($child,0x111,40382,0)
        [void](Wait-For {
            $current=[CaptureWindows]::ProfileValue($settings,'Classic Analyzer','Profile')
            $current -and $current -ne $previous
        } 'the next spectrum preset')
    }
    throw "WACUP could not select the $preset visualization."
}
function Start-PoseTrack($name) {
    $slot=[array]::IndexOf($shotOrder,$name)
    if ($slot -lt 0) {throw "No playback track assigned to $name."}
    if ($ui.Send($main,0x400,0,125) -ne $slot -or $ui.Send($main,0x400,0,104) -ne 1) {
        [void]$ui.Send($main,0x400,$slot,121)
        $ui.Post($main,0x111,40045,0)
    }
    try {
        [void](Wait-For {($ui.Send($main,0x400,0,104) -eq 1) -and ($ui.Send($main,0x400,0,125) -eq $slot)} 'playing the assigned track')
    } catch {throw "Could not play $name, slot $slot (playlist length $($ui.Send($main,0x400,0,124)), index $($ui.Send($main,0x400,0,125)), state $($ui.Send($main,0x400,0,104)))."}
    $trackTitle=[string]$pose.queue[$slot].title
    [void](Wait-For {$ui.Text($main).IndexOf($trackTitle,[StringComparison]::OrdinalIgnoreCase) -ge 0} "the selected title $trackTitle")
    try {[void](Wait-For {$ui.Send($main,0x400,0,105) -gt 0 -and $ui.Send($main,0x400,0,125) -eq $slot} 'decoder playback clock' 12)}
    catch {throw "Decoder stalled on $name (time $($ui.Send($main,0x400,0,105)), state $($ui.Send($main,0x400,0,104)), slot $($ui.Send($main,0x400,0,125)), track $($ui.Text($main)))."}
}
function Save-Shot($name,$only=[IntPtr]::Zero) {
    $slot=[array]::IndexOf($shotOrder,$name)
    Start-PoseTrack $name
    $preset=Set-TrackEqualizer $pose.queue[$slot]
    if ($visualizer -ne [IntPtr]::Zero -and $only -eq [IntPtr]::Zero) {
        $visualPreset=$visualPresets[$slot%$visualPresets.Count]
        Set-AnalyzerPreset $visualizer $visualPreset
    }
    if ($name -in 'classic-modern-album-tracks','classic-modern-folder-contents','classic-modern-playlist-tracks') {
        $ui.FitBackButton((Find-Form 1003))
    }
    if ($only -eq [IntPtr]::Zero) {
        $browser=Find-Form 1003
        if ($browser -and $ui.ControlText($ui.Item($browser,1003)).Length) {
            $list=$ui.Item($browser,1006)
            if ([CaptureWindows]::IsWindowVisible($ui.Item($browser,1043))) {$list=$ui.Item($browser,1043)}
            [void](Wait-For {$ui.Send($list,0x1004,0,0) -gt 1} "multiple visible search results for $name")
        }
    }
    if ($name -match '(-albums|-favorites|-album-search|-olive-playback)$') {
        $view=Wait-For {Find-Form 1003} 'cover browser'
        $allowed=$null
        $firstIndex=0
        if ($name.EndsWith('-albums')) {
            $firstIndex=[int]$pose.albumStart
            $allowed=[string[]]$pose.albumShowcase.name
            [void](Wait-For {$ui.TopAlbum($ui.Item($view,1006),$firstIndex)} 'the unstarred most-played album row')
        } elseif ($name -in 'classic-modern-favorites','classic-modern-olive-playback') {
            Command $view 1510
            [void](Wait-For {$ui.ControlText($ui.Item($view,1000)) -eq 'Song count'} 'Favorites sorted by song count')
            $allowed=[string[]]$pose.favoriteAlbums.name
        }
        try {[void](Wait-For {$ui.CoversPainted($ui.Item($view,1006),$allowed,$firstIndex)} 'all visible covers to paint' 35)}
        catch {throw "$($_.Exception.Message) $($ui.CoverStatus)"}
        if ($name -eq 'classic-modern-olive-playback') {
            if ($ui.CoversAcross -ne 6 -or $ui.FullCovers -ne 12 -or [Math]::Abs($ui.BottomArtworkFraction-0.77) -gt 0.045) {
                throw ("Olive shows "+$ui.CoversAcross+" covers across, "+$ui.FullCovers+" complete, and "+$ui.BottomArtworkFraction.ToString('P0')+" of the bottom covers.")
            }
        }
        if ($name -eq 'big-bento-modern-albums') {
            if ($ui.FullCovers -le 4) {throw 'Big Bento must show more than four complete album tiles.'}
            $reference=Join-Path $run 'now-playing-cover.jpg'
            Get-DemoAsset 'getCoverArt' $pose.queue[$slot].coverArt $reference
            if (!$ui.BentoArtwork($layoutPlayer,$reference)) {
                # Wasabi asks for both embedded covers at track start. Reload after the native sidecar is ready.
                $marker=Wait-For {
                    Get-ChildItem -LiteralPath (Join-Path $profile 'Plugins\Tangerine Stream\items') -File -Recurse -Filter '*.tstream' |
                        Where-Object {$_.BaseName.IndexOf($pose.queue[$slot].title,[StringComparison]::OrdinalIgnoreCase) -ge 0} | Select-Object -First 1
                } 'the now-playing track reference'
                $coverPath=Join-Path $marker.DirectoryName 'cover.jpg'
                if (!(Test-Path -LiteralPath $coverPath)) {[IO.File]::Copy($reference,$coverPath)}
                $ui.Post($main,0x111,40047,0)
                [void](Wait-For {$ui.Send($main,0x400,0,104) -eq 0} 'stopped before reloading the track artwork')
                Start-PoseTrack $name
                try {[void](Wait-For {$ui.BentoArtwork($layoutPlayer,$reference)} 'both Big Bento playback covers' 15)}
                catch {throw "$($_.Exception.Message) $($ui.ArtworkStatus)"}
            }
        }
    }
    # Let the skin return from its temporary EQ adjustment text to the song title.
    Start-Sleep -Milliseconds 1800
    if ($only -eq [IntPtr]::Zero) {
        [void](Wait-For {
            $unexpected = @(Get-Frames | Where-Object {!$placements.ContainsKey($_.ToInt64())})
            return $unexpected.Count -eq 0
        } 'WACUP track notification to close' 20)
    }
    $seconds=$seekPositions[$slot]
    [void](Wait-For {$ui.Send($main,0x400,0,105) -gt 0 -and $ui.Send($main,0x400,0,125) -eq $slot} 'decoder playback clock')
    $seekResult=$ui.Send($main,0x400,($seconds*1000),106)
    try {
        [void](Wait-For {
            $ms=$ui.Send($main,0x400,0,105)
            $ms -ge 1000*$seconds -and $ms -lt 1000*($seconds+2)
        } 'assigned playback position' 8)
    } catch {throw "Could not seek $name to ${seconds}s (result $seekResult, time $($ui.Send($main,0x400,0,105))ms, index $($ui.Send($main,0x400,0,125)), state $($ui.Send($main,0x400,0,104)), title $($ui.Text($main)))."}
    if ($only -eq [IntPtr]::Zero -and $skin -in 'Winamp Classic Modern','Olive') {
        $duration=[double]$pose.queue[$slot].duration
        $seekTimer=[Diagnostics.Stopwatch]::StartNew()
        do {
            $shown=$ui.ClassicModernSeekFraction($layoutPlayer)
            $expected=$ui.Send($main,0x400,0,105)/(1000*$duration)
            if ([Math]::Abs($shown-$expected) -lt 0.025) {break}
            if ($seekTimer.Elapsed.TotalSeconds -gt 8) {throw "Seek thumb for $name shows $shown, expected $expected."}
            Start-Sleep -Milliseconds 100
        } while ($true)
    }
    Start-Sleep -Milliseconds 250
    if ($visualizer -ne [IntPtr]::Zero -and $only -eq [IntPtr]::Zero) {
        [void](Wait-For {$ui.SpectrumMoving($ui.Root($visualizer))} "the $visualPreset spectrum to animate" 8)
    }
    if ($name -eq 'add-server') {
        $ui.ActivateDialog($only,$ui.Item((Find-Form 1021),1021))
    }
    if ($only -eq [IntPtr]::Zero -and $browser) {
        foreach ($list in $ui.Windows($browser)) {
            if ($ui.Class($list) -ne 'SysListView32' -or ![CaptureWindows]::IsWindowVisible($list)) {continue}
            $columns=$ui.FitPopulatedColumns($list)
            if ($columns) {Write-Log "$name columns - $columns"}
        }
    }
    [array]$frames = if ($only -ne [IntPtr]::Zero) { $only } else { Get-Frames }
    if (!$frames) {throw 'WACUP has no visible frames.'}
    [array]::Reverse($frames)
    $bounds = [Drawing.Rectangle]::Empty
    $boxes = @{}
    foreach ($window in $frames) {
        $rect = $ui.Rect($window)
        if ($only -eq [IntPtr]::Zero -and $placements.ContainsKey($window.ToInt64())) {
            $rect.Location = $placements[$window.ToInt64()]
        }
        $boxes[$window.ToInt64()]=$rect
        $bounds=if ($bounds.IsEmpty) {$rect} else {[Drawing.Rectangle]::Union($bounds,$rect)}
    }
    if ($only -eq [IntPtr]::Zero) {
        $covered=0
        foreach ($window in $frames) {
            $r=$boxes[$window.ToInt64()]
            $covered+=$r.Width*$r.Height
            foreach ($other in $frames) {
                if ($other.ToInt64() -le $window.ToInt64()) {continue}
                $overlap=[Drawing.Rectangle]::Intersect($r,$boxes[$other.ToInt64()])
                if ($overlap.Width*$overlap.Height -gt 0) {throw "Overlapping WACUP frames in $name."}
            }
        }
        if ($covered -ne $bounds.Width*$bounds.Height) {
            throw ("Uneven outside edges in $name. "+(($frames|ForEach-Object {$ui.Text($_)+' '+$boxes[$_.ToInt64()]}) -join ' | '))
        }
    }
    $canvas = New-Object Drawing.Bitmap $bounds.Width,$bounds.Height
    try {
        $g = [Drawing.Graphics]::FromImage($canvas)
        try {
            foreach ($window in $frames) {
                $rect = $boxes[$window.ToInt64()]
                $image = $ui.Print($window)
                $clip = $ui.Shape($window)
                try {
                    $x = $rect.X-$bounds.X; $y = $rect.Y-$bounds.Y
                    $clip.Translate($x,$y); $g.Clip=$clip
                    $g.DrawImageUnscaled($image,$x,$y)
                } finally { $clip.Dispose(); $image.Dispose() }
            }
        } finally { $g.Dispose() }
        $canvas.Save((Join-Path $shots ($name+'.png')),[Drawing.Imaging.ImageFormat]::Png)
        Write-Log ("Captured $name ("+$bounds.Width+' x '+$bounds.Height+') playing '+$pose.queue[$slot].title+' at '+$seconds+'s, EQ '+$preset)
    } finally { $canvas.Dispose() }

}

function Get-Frame($title) {
    foreach ($h in Get-Frames) {
        if ($ui.Text($h) -eq $title) { return $h }
    }
}
function Set-ModernProfile($profile,$skin) {
    $olive = $skin -eq 'Olive'
    # Classic Modern's main, equalizer and playlist are a 550px stack.
    # The library's stock anchor is one column right and one player-height down.
    $windows = if ($olive) {
        @(@('main','Main Window','normal','32,32,550,243'),
          @('PLEdit','Playlist Editor','normalpl','32,275,550,246'),
          @('MLibrary','Media Library','normal','32,521,1500,784'),
          @('winamp.albumart','Album Art','normal','1072,32,460,489'),
          @('resizable_nostatus','Lyrics','resizable_nostatus','582,32,490,489'))
    } else {
        @(@('main','Main Window','normal','32,32,550,242'),
          @('equalizer','Equalizer','normaleq','32,274,550,242'),
          @('PLEdit','Playlist Editor','normalpl','32,516,550,242'),
          @('MLibrary','Media Library','normal','582,274,984,484'),
          @('winamp.albumart','Album Art','normal','582,32,492,242'),
          @('resizable_nostatus','Lyrics','resizable_nostatus','1074,32,492,242'))
    }
    $layouts = foreach ($w in $windows) {
        '<section name="Container:{0}"><section name="{1}"><section name="Layout:{2}"><entry name="rect" value="{3}"/><entry name="restoredrect" value="{3}"/><entry name="r" value="1.000000"/><entry name="maximized" value="0"/></section></section></section>' -f $w
    }
    $tint = if ($olive) {'<section name="Color Themes"><entry name="Winamp Classic Modern" value="++ Olive garden"/></section>'} else {''}
    $xml = '<WasabiXML><configuration><section name="{00000000-0000-0000-0000-000000000000}">'+$tint+
        '<section name="Skin:Winamp Classic Modern">'+($layouts -join '')+'</section></section></configuration></WasabiXML>'
    $document = [xml]('<?xml version="1.0"?>'+$xml)
    $document.Save((Join-Path $profile 'studio.xnf'))
}
function Set-Layout($skin) {
    function Place($h,[int]$x,[int]$y,[int]$width,[int]$height) {
        $ui.Place($h,$x,$y,$width,$height)
        $placements[$h.ToInt64()]=[Drawing.Point]::new($x,$y)
    }
    if ($skin -eq 'Big Bento Modern') {
        $frame = $ui.Root($tree)
        Place $frame 32 32 1150 720
        $lyrics=Get-Frame 'Lyrics'
        if ($lyrics) {Place $lyrics 1182 32 320 720}
        return $frame
    }
    if ($classic) {
        $library = $ui.Root($tree)
        $mainRect = $ui.Rect($main)
        $editor = Get-Top 'Winamp PE'
        $equalizer = Get-Top 'Winamp EQ'
        foreach ($h in $main,$equalizer,$editor) {$ui.Show($h,$true)}
        Place $main 32 32 $mainRect.Width $mainRect.Height
        $y = 32+$mainRect.Height
        $eqSize=$ui.Rect($equalizer)
        Place $equalizer 32 $y $mainRect.Width $eqSize.Height
        $y+=$eqSize.Height
        Place $editor 32 $y $mainRect.Width 203
        $y+=$ui.Rect($editor).Height
        $lyrics=Get-Frame 'Lyrics'
        if ($lyrics) {Place $lyrics 32 $y $mainRect.Width 162; $y+=$ui.Rect($lyrics).Height}
        Place $library (32+$mainRect.Width) 32 900 ($y-32)
        return $main
    }
    $library = Get-Frame 'Media Library'
    $playlist = Get-Frame 'Playlist Editor'
    $playerFrame = Get-Frame 'Main Window'
    if (!$library -or !$playlist -or !$playerFrame) { throw 'Classic Modern layout is incomplete.' }
    $playerSize = $ui.Rect($playerFrame)
    $x = 32; $y = 32
    if ($skin -eq 'Olive') {
        $eq=Get-Frame 'Equalizer'
        if ($eq) {$ui.Show($eq,$false)}
        Place $playerFrame $x $y 550 243
        Place $playlist $x ($y+243) 550 246
        $lyrics = Wait-For { Get-Frame 'Lyrics' } 'Lyrics window'
        $art = Wait-For { Get-Frame 'Album Art' } 'Album Art window'
        Place $lyrics ($x+550) $y 490 489
        Place $art ($x+1040) $y 460 489
        Place $library $x ($y+489) 1500 784
    } else {
        $right = $x+$playerSize.Width
        $width = 1100
        $libraryHeight = 630
        Place $playerFrame $x $y $playerSize.Width $playerSize.Height
        Place (Get-Frame 'Equalizer') $x ($y+$playerSize.Height) $playerSize.Width $playerSize.Height
        Place $playlist $x ($y+2*$playerSize.Height) $playerSize.Width ($libraryHeight-$playerSize.Height)
        Place (Get-Frame 'Album Art') $right $y $playerSize.Height $playerSize.Height
        $visWidth=360
        Place (Get-Frame 'Lyrics') ($right+$playerSize.Height) $y ($width-$playerSize.Height-$visWidth) $playerSize.Height
        Place $ui.Root($visualizer) ($right+$width-$visWidth) $y $visWidth $playerSize.Height
        Place $library $right ($y+$playerSize.Height) $width $libraryHeight
    }
    return $playerFrame
}
function Save-Category($category,$name,[bool]$asList=$true,[bool]$allowEmpty=$false) {
    Select-View $category
    $form = Wait-For { Find-Form 1003 } $category
    if ($asList) { Command $form 1051 }
    [void](Wait-Library $allowEmpty)
    Save-Shot $name
}
function Search-Album {
    Select-View 'Albums'
    $form = Wait-For { Find-Form 1003 } 'album browser'
    $ui.SetText($ui.Item($form,1003),$pose.album)
    Command $form 1004
    [void](Wait-For { $ui.Send($ui.Item($form,1006),0x1004,0,0) -eq 1 } 'featured album search')
    Click-First $ui.Item($form,1006)
    return $form
}
function Set-DemoQueue {
    if ($ui.Send($main,0x400,0,124) -ne $pose.queue.Count) {
        [void]$ui.Send($main,0x400,0,101)
        Select-View 'Tracks'
        $view=Wait-For {Find-Form 1003} 'track browser'
        Command $view 1051
        for ($i=0;$i -lt $pose.queue.Count;++$i) {
            $song=$pose.queue[$i]
            $ui.SetText($ui.Item($view,1003),$song.title)
            Command $view 1004
            [void](Wait-For {$ui.SelectListItem($ui.Item($view,1006),$song.title)} ('queue track '+$song.title))
            Command $view 1010
            [void](Wait-For {$ui.Send($main,0x400,0,124) -eq $i+1} 'playlist enqueue')
            if ($shotOrder[$i] -eq 'classic-modern-offline-cache') {Command $view 1011}
        }
    }
    Select-View 'Albums'
    $view=Wait-For {Find-Form 1003} 'album grid'
    $list=$ui.Item($view,1006)
    Command $view $(if ($skin -eq 'Big Bento Modern') {1601} else {1602})
    Command $view 1504
    [void](Wait-For {$ui.ControlText($ui.Item($view,1000)) -eq 'Most played'} 'Most played album sort')
    [void](Wait-For {
        $count=$ui.Send($list,0x1004,0,0)
        if ($count -ge $pose.albums) {return $true}
        [void]$ui.Send($list,0x1013,($count-1),0)
        return $false
    } 'complete album count' 40)
    [void]$ui.Send($list,0x1013,0,0)
}
function New-Intro($index) {
    $song=$pose.queue[$index]
    $filename=($song.artist+' - '+$song.title) -replace '[<>:"/\\|?*\x00-\x1f]','_'
    $path=Join-Path $run ($filename+'.wav')
    Get-DemoAsset 'stream' $song.id $path
    return $path
}
function Get-DemoAsset($method,$id,$path) {
    $salt=[guid]::NewGuid().ToString('N')
    $md5=[Security.Cryptography.MD5]::Create()
    try {
        $token=[BitConverter]::ToString($md5.ComputeHash($utf8.GetBytes($demoPassword+$salt))).Replace('-','').ToLowerInvariant()
        $url=$api+'/rest/'+$method+'.view?size=200&u='+$demoUser+'&s='+$salt+'&t='+$token+'&id='+[Uri]::EscapeDataString($id)
        Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $path
    } finally {$md5.Dispose()}
}
function Pose-Player($skin) {
    $classic = $skin -eq 'WACUP Classic'
    $visualizer=[IntPtr]::Zero
    $placements=@{}
    $profile = Join-Path $run 'profile'
    [void][IO.Directory]::CreateDirectory((Join-Path $profile 'Plugins'))
    [IO.File]::WriteAllText((Join-Path $profile 'Plugins\gen_ml.ini'),"[gen_ml_config]"+$nl+'ldivpos=190'+$nl)
    $app = Join-Path $run 'WACUP'
    [IO.File]::WriteAllText((Join-Path $app 'paths.ini'),"[winamp]"+$nl+"inidir=$profile"+$nl+'portable=0'+$nl)
    $skinName = if ($classic) {''} elseif ($skin -eq 'Olive') {'Winamp Classic Modern'} else {$skin}
    $ini = @"
[Winamp]
skin=$skinName
mi_allow_multiple_inst=1
showlibrary=1
newverchk=0
volume=0
outname=out_screenshot.dll
visplugin_name=vis_classic.dll
visplugin_num=0
[gen_lyrics]
lyrics=1
show_art=0
"@
    if ($skin -eq 'Olive' -or $skin -eq 'Winamp Classic Modern') { Set-ModernProfile $profile $skin }
    [IO.File]::WriteAllText((Join-Path $profile 'winamp.ini'),$ini+$nl,$utf8)
    Write-Log "Starting $skin"
    $player = Start-PrivateProcess (Join-Path $app 'wacup.exe') '/NEW' $app
    $ui = New-Object CaptureWindows $player.Id,$env:TS_SHOT_WORKER
    $main = [IntPtr]::Zero
    try {
        $main = Wait-For { Get-Top 'Winamp v1.x' } 'player window' 60
        $tree = Wait-For {
            foreach ($h in $ui.Windows([IntPtr]::Zero)) {
                foreach ($child in $ui.Windows($h)) {
                    if ($ui.Class($child) -eq 'SysTreeView32' -and $ui.SelectTree($child,'Tangerine Stream')) {return $child}
                }
            }
        } 'Media Library tree' 60
        [void](Wait-For {
            [void]$ui.Send($main,0x400,0,122)
            $ui.Send($main,0x400,-666,122) -eq 0
        } 'the player to initialize its volume' 45)
        if (![CaptureWindows]::IsWindowVisible($tree)) { $ui.Post($main,0x111,40379,0) }
        [void](Wait-For { [CaptureWindows]::IsWindowVisible($tree) } 'visible Media Library')
        Start-Sleep -Milliseconds 450
        if ($skin -eq 'Winamp Classic Modern') {
            [IO.File]::WriteAllText((Join-Path $profile 'Plugins\vis_classic.ini'),"[Classic Analyzer]"+$nl+'Profile=Aurora Borealis'+$nl)
            $ui.Post($main,0x111,40192,0)
            $visualizer=Wait-For {
                foreach ($top in $ui.Windows([IntPtr]::Zero)) {
                    foreach ($child in $ui.Windows($top)) {if ($ui.Class($child) -eq 'ClassicSpectrum') {return $child}}
                }
            } 'Classic Spectrum Analyzer'
        }
        [IntPtr]$layoutPlayer=Set-Layout $skin
        if ($skin -eq 'Winamp Classic Modern') {
            $welcome = Wait-For { Find-Form 1001 } 'Add server button'
            [void]$ui.Send($main,0x400,0,101)
            foreach ($i in 0,1) {$ui.EnqueueFile($main,(New-Intro $i))}
            [void](Wait-For {$ui.Send($main,0x400,0,124) -eq 2} 'two silent welcome tracks')
            Save-Shot 'classic-modern-welcome'
            Start-PoseTrack 'add-server'
            $ui.Post($ui.Item($welcome,1001),0xf5,0,0)
            $settings = Wait-For { Find-Form 1021 } 'server settings'
            $ui.SetText($ui.Item($settings,1020),'Demo Server')
            $ui.SetText($ui.Item($settings,1021),'https://tangerine.stream/')
            $ui.SetText($ui.Item($settings,1022),'okeeblow')
            $ui.SetText($ui.Item($settings,1023),'tangerine')
            if (!$ui.Send($ui.Item($settings,1024),0xf0,0,0)) {[void]$ui.Send($ui.Item($settings,1024),0xf5,0,0)}
            # The settings form updates its server list when its selection changes.
            $servers=$ui.Item($settings,1027)
            Command $settings 1001
            [void](Wait-For {$ui.Send($servers,0x1004,0,0) -eq 2} 'the temporary blank server row')
            Command $settings 1034
            [void](Wait-For {$ui.Send($servers,0x1004,0,0) -eq 1 -and
                $ui.ControlText($ui.Item($settings,1020)) -eq 'Demo Server' -and $ui.SelectListItem($servers,'Demo Server')} 'Demo Server in both fields')
            Save-Shot 'add-server' $ui.Root($settings)
            $ui.SetText($ui.Item($settings,1020),'Music Library')
            $ui.SetText($ui.Item($settings,1021),$api)
            $ui.SetText($ui.Item($settings,1022),$demoUser)
            $ui.SetText($ui.Item($settings,1023),$demoPassword)
            $ok = $ui.Item($settings,1)
            if ($ok -eq [IntPtr]::Zero) { $ok=$ui.Item($ui.Root($settings),1) }
            $ui.Post($ok,0xf5,0,0)
        }
        $form = Wait-Library
        Set-DemoQueue
        if ($skin -eq 'Winamp Classic Modern') {
            Save-Shot 'classic-modern-albums'
            $form=Wait-Library
            $ui.SetText($ui.Item($form,1003),$pose.albumQuery)
            Command $form 1004
            [void](Wait-For {$ui.Send($ui.Item($form,1006),0x1004,0,0) -eq $pose.albumMatches -and $pose.albumMatches -gt 1} 'complete multi-album search')
            Save-Shot 'classic-modern-album-search'
            $form=Search-Album
            Command $form 1008
            [void](Wait-For {$ui.ControlText($ui.Item($form,1013)).Contains($pose.album)} 'album tracks')
            [void](Wait-Library)
            Click-First $ui.Item($form,1006)
            Command $form 1011
            Save-Shot 'classic-modern-album-tracks'
            foreach ($pair in @(@('Genres','genres'),@('Album Artists','album-artists'),@('Artists','artists'),@('Tracks','tracks'),
                                 @('Folders','folders'),@('Playlists','playlists'))) {
                Save-Category $pair[0] ('classic-modern-'+$pair[1]) $true ($pair[0] -eq 'Playlists')
                if ($pair[0] -eq 'Folders' -or ($pair[0] -eq 'Playlists' -and $pose.playlist)) {
                    $form=Wait-Library
                    if ($pair[0] -eq 'Playlists') {
                        $ui.SetText($ui.Item($form,1003),$pose.playlist); Command $form 1004
                        [void](Wait-For {$ui.SelectListItem($ui.Item($form,1006),$pose.playlist)} 'the featured playlist')
                    } else {Click-First $ui.Item($form,1006)}
                    Command $form 1008
                    [void](Wait-For {$ui.ControlText($ui.Item($form,1013)).Contains('  |  ')} 'opened contents view')
                    Command $form 1051
                    [void](Wait-Library)
                    Save-Shot $(if ($pair[0] -eq 'Folders') {'classic-modern-folder-contents'} else {'classic-modern-playlist-tracks'})
                }
            }
            Save-Category 'Favorites' 'classic-modern-favorites' $false
            Select-View 'Tracks'
            $form=Wait-Library
            $ui.SetText($ui.Item($form,1003),$pose.filter)
            Command $form 1004
            [void](Wait-For {$count=$ui.Send($ui.Item($form,1006),0x1004,0,0); $count -gt 1 -and $count -le 23} 'multi-track catalog search')
            Command $form 1053
            [void](Wait-For {[CaptureWindows]::IsWindowVisible($ui.Item($form,1040))} 'filter panes')
            [void](Wait-Library)
            Save-Shot 'classic-modern-filtered-tracks'
            Start-PoseTrack 'edit-view-mode'
            Command $form 1050
            $dialog=Wait-For {Find-Form 1060} 'view mode editor'
            Save-Shot 'edit-view-mode' $ui.Root($dialog)
            $ui.Post($ui.Root($dialog),0x10,0,0)
            [void](Wait-For {!(Find-Form 1060)} 'closed view mode editor')
            Start-PoseTrack 'edit-columns'
            Command $form 1058
            $dialog=Wait-For {Find-Form 1067} 'columns editor'
            Save-Shot 'edit-columns' $ui.Root($dialog)
            $ui.Post($ui.Root($dialog),0x10,0,0)
            [void](Wait-For {!(Find-Form 1067)} 'closed columns editor')
            Select-View 'Offline Cache'
            $form=Wait-Library
            Command $form 1015
            [void](Wait-For {$ui.ControlText($ui.Item($form,1013)).StartsWith('Offline')} 'offline mode')
            Save-Shot 'classic-modern-offline-cache'
            Command $form 1015
            [void](Wait-For {!$ui.ControlText($ui.Item($form,1013)).StartsWith('Offline')} 'online library')
        } elseif ($skin -eq 'Olive') {
            Select-View 'Favorites'
            [void](Wait-Library)
            Save-Shot 'classic-modern-olive-playback'
        } else {
            Save-Shot $(if ($classic) {'wacup-classic-albums'} else {'big-bento-modern-albums'})
        }
    } finally {
        try {
            if ($main -ne [IntPtr]::Zero -and !$player.HasExited) {
                $ui.Post($main,0x111,40047,0)
                $ui.Post($main,0x111,40001,0)
                [void]$player.WaitForExit(6500)
            }
        } finally {
            if (!$player.HasExited) { $player.Kill(); $player.WaitForExit() }
            $player.Dispose()
        }
    }
}
function Invoke-Capture {
    [void][CaptureWindows]::SetProcessDPIAware()
    Install-PrivateWacup
    $factoryPresets=Get-FactoryPresets
    Build-Tools
    $plugins=Join-Path $run 'WACUP\Plugins'
    if (@(Get-ChildItem -LiteralPath $plugins -Filter 'out_*.dll').Count) {throw 'The capture player must use its silent output sink exclusively.'}
    Copy-Item -LiteralPath (Join-Path $build 'out_screenshot.dll') -Destination $plugins
    $visualPresets=@('Aurora Borealis','Classic LED','Blue Flames','City Night','Rainbow','Green','Lightning','Twilight',
                     'Purple Neon','Default Red & Yellow','Lavender Pink Tips','Northern Lights','Blue on Dark-Orange',
                     'Flames','ChaNinja','Blue on Grey','Trippy')
    $env:APPDATA=Join-Path $run 'Roaming'
    $env:LOCALAPPDATA=Join-Path $run 'Local'
    $env:TEMP=$env:TMP=Join-Path $run 'Temp'
    $server = Start-PrivateProcess $imposter ('serve "'+$demo+'"') $run $true $true
    try {
        $demoUser = 'demo-'+[guid]::NewGuid().ToString('N')
        $demoPassword = [guid]::NewGuid().ToString('N')
        Write-Login $server $demoUser $demoPassword
        $line = $server.StandardOutput.ReadLineAsync()
        if (!$line.Wait(10000) -or $line.Result -notmatch '^\d+$') {throw 'The demo server did not provide a loopback port.'}
        $api = 'http://127.0.0.1:'+$line.Result
        $login = @{username=$demoUser;password=$demoPassword}|ConvertTo-Json
        $token = (Invoke-RestMethod -Method Post -Uri ($api+'/auth/login') -ContentType 'application/json' -Body $login).token
        $pose = Invoke-RestMethod ($api+'/api/pose') -Headers @{'x-nd-authorization'='Bearer '+$token}
        $shotOrder=@('classic-modern-welcome','add-server','classic-modern-albums','classic-modern-album-search',
            'classic-modern-album-tracks','classic-modern-genres','classic-modern-album-artists','classic-modern-artists',
            'classic-modern-tracks','classic-modern-folders','classic-modern-folder-contents','classic-modern-playlists')
        if ($pose.playlist) {$shotOrder+='classic-modern-playlist-tracks'}
        $shotOrder+=@('classic-modern-favorites','classic-modern-filtered-tracks','edit-view-mode','edit-columns',
            'classic-modern-offline-cache','big-bento-modern-albums','wacup-classic-albums','classic-modern-olive-playback')
        if ($shotOrder.Count -ne $pose.queue.Count) {throw 'Refresh the demo database to match the current screenshot poses.'}
        $seekPositions=@()
        for ($i=0;$i -lt $pose.queue.Count;++$i) {
            $duration=[Math]::Min(1800,[Math]::Floor($pose.queue[$i].duration))
            $percent=12+(24+37*$i)%79
            $position=[int][Math]::Floor($duration*$percent/100)
            if ($shotOrder[$i] -eq 'big-bento-modern-albums') {$position=[int]$pose.bentoSeek}
            while ($position -in $seekPositions) {$position=1+($position%($duration-2))}
            $seekPositions+=$position
        }
        foreach ($skin in 'Winamp Classic Modern','Big Bento Modern','WACUP Classic','Olive') { Pose-Player $skin }
    } finally {
        if (!$server.HasExited) {$server.Kill(); $server.WaitForExit()}
        $server.Dispose()
    }
}
function Restore-Publish {
    if (!(Test-Path -LiteralPath $publishDir)) {return}
    $stage=Get-Item -LiteralPath $publishDir -Force
    if ($stage.Parent.FullName -ne $root -or $stage.Name -ne 'build-screenshots-publish' -or
        ($stage.Attributes -band [IO.FileAttributes]::ReparsePoint)) {throw 'Invalid gallery staging path.'}
    $journal=Join-Path $publishDir 'new-files'
    if ((Test-Path -LiteralPath $journal) -and !(Test-Path -LiteralPath (Join-Path $publishDir 'complete'))) {
        foreach ($name in [IO.File]::ReadAllLines($journal)) {
            if ($name -notmatch '^[a-z0-9-]+\.png$') {throw 'Invalid screenshot filename in the publish journal.'}
            [IO.File]::Delete((Join-Path $gallery $name))
        }
        foreach ($file in Get-ChildItem -LiteralPath (Join-Path $publishDir 'backup') -File -Filter '*.png') {
            [IO.File]::Copy($file.FullName,(Join-Path $gallery $file.Name),$true)
        }
    }
    Remove-Item -LiteralPath $stage.FullName -Recurse -Force
}
function Publish-Gallery {
    [void][IO.Directory]::CreateDirectory($gallery)
    $ready=Join-Path $publishDir 'ready'
    $backup=Join-Path $publishDir 'backup'
    foreach ($directory in $ready,$backup) {[void][IO.Directory]::CreateDirectory($directory)}
    $newFiles=@()
    $files=@(Get-ChildItem -LiteralPath $shots -File -Filter '*.png')
    try {
        foreach ($file in $files) {
            [IO.File]::Copy($file.FullName,(Join-Path $ready $file.Name))
            if (!(Test-Path -LiteralPath (Join-Path $gallery $file.Name))) {$newFiles+=$file.Name}
        }
        $journal=Join-Path $publishDir 'new-files'
        [IO.File]::WriteAllLines(($journal+'.tmp'),[string[]]$newFiles)
        [IO.File]::Move(($journal+'.tmp'),$journal)
        foreach ($file in $files) {
            $output=Join-Path $gallery $file.Name
            $inputFile=Join-Path $ready $file.Name
            if (Test-Path -LiteralPath $output) {[IO.File]::Replace($inputFile,$output,(Join-Path $backup $file.Name))}
            else {[IO.File]::Move($inputFile,$output)}
        }
        $optional='classic-modern-playlist-tracks.png'
        if ($optional -notin $files.Name -and (Test-Path -LiteralPath (Join-Path $gallery $optional))) {
            [IO.File]::Move((Join-Path $gallery $optional),(Join-Path $backup $optional))
        }
        [IO.File]::WriteAllText((Join-Path $publishDir 'complete'),'ok')
    } finally {Restore-Publish}
}
if ($env:TS_SHOT_GUARD) {
    # The guard has its own hidden console. Closing the batch window closes the
    # capture job, then the guard can remove the released profiles and binaries.
    $owner=Get-Process -Id ([int]$env:TS_SHOT_GUARD) -ErrorAction SilentlyContinue
    if ($owner -and $owner.StartTime.ToUniversalTime().Ticks -eq [long]$env:TS_SHOT_OWNER_TICKS) {
        $owner.WaitForExit()
    }
    $guardRun=$env:TS_SHOT_RUN
    if (Test-Path -LiteralPath $guardRun) {Remove-RunDirectory ([IO.Path]::GetTempPath().TrimEnd('\')) ([IO.Path]::GetFileName($guardRun))}
    exit 0
}
if ($env:TS_SHOT_WORKER) {
    $run=$env:TS_SHOT_RUN
    $shots=Join-Path $run 'shots'
    try {
        if ($action -eq 'refresh') {Invoke-Refresh} else {Invoke-Capture}
        [IO.File]::WriteAllText((Join-Path $run 'complete'),'ok')
        exit 0
    }
    catch { Write-Log ($_.Exception.Message+$nl+$_.ScriptStackTrace); exit 1 }
}
$gate = New-Object Threading.Mutex $false,'Local\TangerineStream-Screenshots'
$locked=$false
$session=$null
$run=$null
$pipe=$null
$secret=$null
$gallery=Join-Path $root 'screenshots'
$publishDir=Join-Path $root 'build-screenshots-publish'
try {
    try { $locked=$gate.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked=$true }
    if (!$locked) {throw 'Another screenshot run is active.'}
    Restore-Publish
    $parent = [IO.Path]::GetTempPath().TrimEnd('\')
    foreach ($old in Get-ChildItem -LiteralPath $parent -Directory -Filter 'TangerineShots-*') {
        if ($old.Name -match '^TangerineShots-[0-9a-f]{32}$' -and (Test-Path -LiteralPath (Join-Path $old.FullName '.capture-workspace'))) {
            Remove-RunDirectory $parent $old.Name
        }
    }
    $name = 'TangerineShots-'+[guid]::NewGuid().ToString('N')
    $run=Join-Path $parent $name
    $shots=Join-Path $run 'shots'
    [void][IO.Directory]::CreateDirectory($shots)
    [IO.File]::WriteAllText((Join-Path $run '.capture-workspace'),'Tangerine Stream')
    $env:TS_SHOT_RUN=$run
    $env:TS_SHOT_GUARD=[string]$PID
    $env:TS_SHOT_OWNER_TICKS=[string][Diagnostics.Process]::GetCurrentProcess().StartTime.ToUniversalTime().Ticks
    try {
        $guard=Start-PrivateProcess $env:ComSpec ('/d /c ""'+$env:TS_SHOT_FILE+'" --no-pause"') $root
        $guard.Dispose()
    } finally {Remove-Item Env:TS_SHOT_GUARD,Env:TS_SHOT_OWNER_TICKS}
    $workerArgs='WACUP '+$compiler+' --no-pause'
    if ($action -eq 'refresh') {
        $secret = if ([Console]::IsInputRedirected) {
            ConvertTo-SecureString ([Console]::ReadLine()) -AsPlainText -Force
        } else {Read-Host 'Navidrome password for okeeblow at https://music.cooltrainer.org' -AsSecureString}
        $security=[IO.Pipes.PipeSecurity]::new()
        $sid=[Security.Principal.WindowsIdentity]::GetCurrent().User
        $security.AddAccessRule([IO.Pipes.PipeAccessRule]::new($sid,'FullControl','Allow'))
        $pipe=[IO.Pipes.NamedPipeServerStream]::new($name,'Out',1,'Byte','Asynchronous',4096,4096,$security)
        $connected=$pipe.WaitForConnectionAsync()
        $workerArgs+=' rebuild-db'
    }
    $session=New-Object CaptureDesktop $name
    $env:TS_SHOT_WORKER=$name
    $worker=$session.Start($env:ComSpec, ('"'+$env:ComSpec+'" /d /c ""'+$env:TS_SHOT_FILE+'" '+$workerArgs+'"'),$root)
    if ($pipe) {
        if (!$connected.Wait(25000)) {throw 'The database export worker did not connect.'}
        $credential=[Management.Automation.PSCredential]::new('okeeblow',$secret)
        $writer=[IO.StreamWriter]::new($pipe,$utf8)
        try {$writer.WriteLine($credential.GetNetworkCredential().Password)} finally {$writer.Dispose()}
        $credential=$null
    }
    $log=Join-Path $run 'progress'
    $sent=0
    do {
        $done=$worker.WaitForExit(500)
        if (Test-Path -LiteralPath $log) {
            $lines=[IO.File]::ReadAllLines($log)
            while ($sent -lt $lines.Length) {Write-Host $lines[$sent]; ++$sent}
        }
    } while (!$done)
    if ($worker.ExitCode -or !(Test-Path -LiteralPath (Join-Path $run 'complete'))) {throw "WACUP $action failed."}
    if ($action -eq 'refresh') {
        [void][IO.Directory]::CreateDirectory($publishDir)
        try {
            $staged=Join-Path $publishDir 'demo.sqlite3'
            [IO.File]::Copy((Join-Path $run 'demo.sqlite3'),$staged)
            if (Test-Path -LiteralPath $demo) {[IO.File]::Replace($staged,$demo,(Join-Path $publishDir 'previous.sqlite3'))}
            else {[IO.File]::Move($staged,$demo)}
        } finally {Restore-Publish}
    } else {
        Publish-Gallery
        Write-Host ("Saved screenshots to "+(Join-Path $root 'screenshots'))
    }
} finally {
    if ($pipe) {$pipe.Dispose()}
    if ($secret) {$secret.Dispose()}
    if ($session) {$session.Dispose()}
    try {if ($run) {Remove-RunDirectory $parent $name}}
    finally {if ($locked) {$gate.ReleaseMutex()}; $gate.Dispose()}
}
