using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Automation;
using Accessibility;

// Every window operation verifies the WACUP process and its private Windows desktop.
public sealed partial class CaptureWindows
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
    [StructLayout(LayoutKind.Sequential)] struct GuiThread {
        public int size,flags;
        public IntPtr active,focus,capture,menu,move,caret;
        public Bounds caretRect;
    }
    [DllImport("user32.dll",SetLastError=true)] static extern bool GetGUIThreadInfo(uint thread,ref GuiThread state);
    [DllImport("user32.dll")] static extern bool IsChild(IntPtr parent,IntPtr child);
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
    public void FocusFrame(IntPtr frame,IntPtr field) {
        Check(frame);Check(field);
        if(field!=frame && !IsChild(frame,field)) throw new InvalidOperationException("Focus target is outside the capture frame.");
        int owner;
        uint from=GetCurrentThreadId(),to=GetWindowThreadProcessId(frame,out owner);
        if(!AttachThreadInput(from,to,true)) throw new Win32Exception();
        try {
            SetWindowPos(frame,IntPtr.Zero,0,0,0,0,3);
            SetActiveWindow(frame); SetFocus(field);
            var focused=GetFocus();
            if(GetActiveWindow()!=frame || (focused!=field && !IsChild(field,focused)))
                throw new InvalidOperationException("Could not activate "+Text(frame)+" and focus its "+Class(field)+" control.");
            Send(frame,0x86,1,0); // WM_NCACTIVATE paints the active nonclient title.
        } finally {AttachThreadInput(from,to,false);}
    }
    public void CheckFocus(IntPtr frame,IntPtr field) {
        Check(frame); Check(field);
        int owner;
        uint thread=GetWindowThreadProcessId(field,out owner);
        var state=new GuiThread {size=Marshal.SizeOf(typeof(GuiThread))};
        if(!GetGUIThreadInfo(thread,ref state)) throw new Win32Exception();
        if(state.active!=frame || (state.focus!=field && !IsChild(field,state.focus))) FocusFrame(frame,field);
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
    public bool ArtworkMatches(IntPtr player,string filename,Rectangle[] panes) {
        using(var expected=new Bitmap(filename))
        using(var frame=Print(player)) {
            foreach(var area in panes) {
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
    public string RowStatus { get; private set; }
    public bool TopReportRow(IntPtr list,int start,int total,string title) {
        Check(list);
        int count=(int)Send(list,0x1004,0,0);
        if(count!=total) {RowStatus="Loaded "+count+" of "+total+" rows.";return false;}
        var guid=typeof(IAccessible).GUID; IAccessible rows;
        if(AccessibleObjectFromWindow(list,0xfffffffc,ref guid,out rows)!=0) return false;
        try {
            var first=rows.get_accName(start+1);
            if(first!=title) {RowStatus="Row "+start+" contains ["+first+"], expected ["+title+"].";return false;}
            Send(list,0x1013,start,0);
            int top=(int)Send(list,0x1027,0,0), x,y,w,h;
            rows.accLocation(out x,out y,out w,out h,start+1);
            if(top!=start) Send(list,0x1014,0,(start-top)*h);
            top=(int)Send(list,0x1027,0,0);
            if(top!=start) {RowStatus="Top row is "+top+", expected "+start+".";return false;}
            var bounds=Rect(list);
            var last=first;
            for(int i=top;i<count;i++) {
                rows.accLocation(out x,out y,out w,out h,i+1);
                if(y>=bounds.Bottom) break;
                last=rows.get_accName(i+1);
                if(String.IsNullOrEmpty(last) || last.Trim(' ','\u2800','\u2060','\u200b').Length==0) {
                    RowStatus="Unpopulated visible row at "+i+" following "+first+".";return false;
                }
            }
            RowStatus="row "+(top+1)+" of "+count+", "+first+" through "+last;
            return true;
        } catch(ArgumentException) {return false;}
        catch(COMException) {return false;}
        finally {Marshal.ReleaseComObject(rows);}
    }
    public string FitPopulatedColumns(IntPtr list,string[] compactColumns) {
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
        var heading=new int[columns];
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
                var widths=new List<int>();
                for(int row=first;row<last;row++) {
                    var cell=grid.GetItem(row,col);
                    if(cell==null || cell.Current.BoundingRectangle.Top>=viewportBottom) continue;
                    string value=cell.Current.Name;
                    if(String.IsNullOrWhiteSpace(value) || value.Trim() == "\u2800") continue;
                    widths.Add((int)Math.Ceiling(g.MeasureString(value,SystemFonts.MessageBoxFont).Width)+18);
                }
                if(widths.Count==0) {report.Add(names[col]+" hidden");continue;}
                widths.Sort();
                heading[col]=(int)Math.Ceiling(g.MeasureString(names[col],SystemFonts.MessageBoxFont).Width)+18;
                bool compact=Array.IndexOf(compactColumns,names[col])>=0;
                // Fit repeated metadata in full. A single long artist credit
                // should not consume space that would display the titles of many tracks.
                minimum[col]=Math.Max(heading[col],widths[compact?widths.Count-1:widths.Count/4]);
                preferred[col]=Math.Max(minimum[col],widths[widths.Count-1-widths.Count/10]);
                flex[col]=compact?0:preferred[col];
                minimumTotal+=minimum[col]; contentTotal+=preferred[col]; flexTotal+=flex[col];
            }
        }
        if(contentTotal==0) throw new InvalidOperationException("The nonempty library table returned no visible values.");
        int space=bounds.right-3, weighted=0, assigned=0;
        if(minimumTotal>space) {
            minimum=heading;
            minimumTotal=0;
            foreach(int pixels in heading) minimumTotal+=pixels;
        }
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
    public Rectangle ChildRect(IntPtr form,IntPtr child) {
        Check(form); Check(child);
        if(!IsChild(form,child)) throw new InvalidOperationException("Expected a child of the specified form.");
        Bounds box; GetWindowRect(child,out box);
        MapWindowPoints(IntPtr.Zero,form,ref box,2);
        return Rectangle.FromLTRB(box.left,box.top,box.right,box.bottom);
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
