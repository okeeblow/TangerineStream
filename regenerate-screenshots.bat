@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "TS_SHOT_FILE=%~f0"
set "TS_SHOT_ARGS=%*"
powershell.exe -NoLogo -NoProfile -Command "$file=[IO.File]::ReadAllText($env:TS_SHOT_FILE); . ([scriptblock]::Create($file.Substring($file.IndexOf([char]10+'# PowerShell')+1)))"
set "ts_result=%errorlevel%"
if "%ts_result%"=="0" goto finished
if defined TS_SHOT_WORKER goto finished
if not defined TS_SHOT_ARGS goto failed
if not "%TS_SHOT_ARGS:--no-pause=%"=="%TS_SHOT_ARGS%" goto finished
:failed
pause
:finished
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
# Gallery poses and window layouts
function Save-Welcome($welcome) {
    $ui.Post($main,0x111,40047,0)
    [void]$ui.Send($main,0x400,0,101)
    [void](Wait-For {$ui.Send($main,0x400,0,104) -eq 0 -and $ui.Send($main,0x400,0,124) -eq 0} 'stopped player and empty playlist')
    Save-Shot 'classic-modern-welcome' -stopped -focusField $ui.Item($welcome,1001)
}
function Connect-DemoServer {
    $welcome = Wait-For { Find-Form 1001 } 'Add server button'
    Save-Welcome $welcome
    $ui.EnqueueFile($main,(New-Intro 0))
    [void](Wait-For {$ui.Send($main,0x400,0,124) -eq 1} 'the Add Server track')
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
    Save-Shot 'add-server' $ui.Root($settings) $ui.Item($settings,1021)
    $ui.SetText($ui.Item($settings,1020),'Music Library')
    $ui.SetText($ui.Item($settings,1021),$api)
    $ui.SetText($ui.Item($settings,1022),$demoUser)
    $ui.SetText($ui.Item($settings,1023),$demoPassword)
    $ok = $ui.Item($settings,1)
    if ($ok -eq [IntPtr]::Zero) { $ok=$ui.Item($ui.Root($settings),1) }
    $ui.Post($ok,0xf5,0,0)
}
function Save-ClassicModernGallery {
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
                         @('Playlists','playlists'))) {
        Save-Category $pair[0] ('classic-modern-'+$pair[1]) $true ($pair[0] -eq 'Playlists')
        if ($pair[0] -eq 'Playlists' -and $pose.playlist) {
            $form=Wait-Library
            $ui.SetText($ui.Item($form,1003),$pose.playlist); Command $form 1004
            [void](Wait-For {$ui.SelectListItem($ui.Item($form,1006),$pose.playlist)} 'the featured playlist')
            Command $form 1008
            [void](Wait-For {$ui.ControlText($ui.Item($form,1013)).Contains('  |  ')} 'opened contents view')
            Command $form 1051
            [void](Wait-Library)
            Save-Shot 'classic-modern-playlist-tracks'
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
    Save-Shot 'edit-view-mode' $ui.Root($dialog) $ui.Item($dialog,1060)
    $ui.Post($ui.Root($dialog),0x10,0,0)
    [void](Wait-For {!(Find-Form 1060)} 'closed view mode editor')
    Start-PoseTrack 'edit-columns'
    Command $form 1058
    $dialog=Wait-For {Find-Form 1067} 'columns editor'
    Save-Shot 'edit-columns' $ui.Root($dialog) $ui.Item($dialog,1067)
    $ui.Post($ui.Root($dialog),0x10,0,0)
    [void](Wait-For {!(Find-Form 1067)} 'closed columns editor')
    Select-View 'Offline Cache'
    $form=Wait-Library
    Command $form 1015
    [void](Wait-For {$ui.ControlText($ui.Item($form,1013)).StartsWith('Offline')} 'offline mode')
    Save-Shot 'classic-modern-offline-cache'
    Command $form 1015
    [void](Wait-For {!$ui.ControlText($ui.Item($form,1013)).StartsWith('Offline')} 'online library')
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

# Private capture session
function Start-PrivateProcess($file,$arguments,$directory,[bool]$stdin=$false,[bool]$stdout=$false,[bool]$stderr=$false) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $file
    $info.Arguments = $arguments
    $info.WorkingDirectory = $directory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $stdin
    $info.RedirectStandardOutput = $stdout
    $info.RedirectStandardError = $stderr
    if ($stdout) {$info.StandardOutputEncoding=$utf8}
    if ($stderr) {$info.StandardErrorEncoding=$utf8}
    return [Diagnostics.Process]::Start($info)
}
function Write-Login($process,$username,$password) {
    $stream = $process.StandardInput.BaseStream
    $bytes = $utf8.GetBytes($username + $nl + $password + $nl)
    try { $stream.Write($bytes,0,$bytes.Length); $stream.Flush() }
    finally { [Array]::Clear($bytes,0,$bytes.Length); $process.StandardInput.Close() }
}
$hostTool=Join-Path $build 'screenshot-host.exe'

function Invoke-Launcher {
    $bootstrap=New-Object Threading.Mutex $false,'Local\TangerineStream-ScreenshotLauncher'
    $locked=$false
    try {
        try {$locked=$bootstrap.WaitOne(0)} catch [Threading.AbandonedMutexException] {$locked=$true}
        if (!$locked) {throw 'Another screenshot run is active.'}
        Write-Host 'Preparing the WACUP screenshot tools...'
        $buildFile=Join-Path $root 'build.bat'
        $builder=Start-PrivateProcess $env:ComSpec ('/d /c ""'+$buildFile+'" '+$compiler+' --screenshot-host --no-pause 2>&1"') $root $false $true
        try {
            $log=$builder.StandardOutput.ReadToEnd()
            $builder.WaitForExit()
            if ($builder.ExitCode) {throw $log}
        } finally {$builder.Dispose()}
        $arguments='run "'+$env:TS_SHOT_FILE+'" '+$compiler+' '+$action+' '+$PID
        $refresh=$action -eq 'refresh'
        $secret=$null
        if ($refresh) {
            $secret=if ([Console]::IsInputRedirected) {ConvertTo-SecureString ([Console]::ReadLine()) -AsPlainText -Force}
                    else {Read-Host 'Navidrome password for okeeblow at https://music.cooltrainer.org' -AsSecureString}
        }
        try {
            $supervisor=Start-PrivateProcess $hostTool $arguments $root $refresh $true $true
            try {
                $errors=$supervisor.StandardError.ReadToEndAsync()
                if ($refresh) {
                    $credential=[Management.Automation.PSCredential]::new('okeeblow',$secret)
                    $bytes=$utf8.GetBytes($credential.GetNetworkCredential().Password+$nl)
                    try {$supervisor.StandardInput.BaseStream.Write($bytes,0,$bytes.Length)}
                    finally {[Array]::Clear($bytes,0,$bytes.Length);$supervisor.StandardInput.Close()}
                }
                while ($null -ne ($line=$supervisor.StandardOutput.ReadLine())) {Write-Host $line}
                $supervisor.WaitForExit()
                if ($supervisor.ExitCode) {throw $errors.Result.Trim()}
            } finally {$supervisor.Dispose()}
        } finally {if ($secret) {$secret.Dispose()}}
    } finally {if ($locked) {$bootstrap.ReleaseMutex()};$bootstrap.Dispose()}
}

if (!$env:TS_SHOT_WORKER) {Invoke-Launcher;exit 0}
$run=$env:TS_SHOT_RUN
$shots=Join-Path $run 'shots'
function Write-Log($value) {[IO.File]::AppendAllText((Join-Path $run 'progress'),$value+$nl,$utf8)}

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
function Invoke-Refresh {
    [Console]::InputEncoding=$utf8
    $password=[Console]::ReadLine()
    if (!$password) {throw 'Missing Navidrome password.'}
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
function Install-PrivateWacup {
    $installer=Get-LatestInstaller
    $sevenZip=(Get-Command 7z.exe -ErrorAction SilentlyContinue).Source
    if (!$sevenZip) {
        $sevenZip=@($env:ProgramW6432,$env:ProgramFiles,${env:ProgramFiles(x86)} | Where-Object {$_} |
            ForEach-Object {Join-Path $_ '7-Zip\7z.exe'} | Where-Object {Test-Path -LiteralPath $_}) | Select-Object -First 1
    }
    if (!$sevenZip) {throw 'Install 7-Zip to unpack WACUP.'}
    & $hostTool unpack $run $installer $sevenZip
    if ($LASTEXITCODE) {throw 'Could not unpack the private WACUP installation.'}
}

$wpf=Join-Path ([Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory()) 'WPF'
$uiAssemblies=@('System.Drawing','Accessibility')+@('UIAutomationClient.dll','UIAutomationTypes.dll','WindowsBase.dll' | ForEach-Object {Join-Path $wpf $_})
$controls=[IO.File]::ReadAllText((Join-Path $tools 'WindowControls.cs'))
$capture=@'
public sealed partial class CaptureWindows {
    [DllImport("user32.dll",SetLastError=true)] static extern bool PrintWindow(IntPtr window,IntPtr dc,uint flags);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr window,uint attribute,out Bounds bounds,int size);
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
    public string DialogBounds {get;private set;}
    static bool EmptyFrameEdge(Bitmap image,int x,int y,int dx,int dy,int length) {
        for(int i=0;i<length;i++,x+=dx,y+=dy)
            if(image.GetPixel(x,y).ToArgb()!=Color.Black.ToArgb()) return false;
        return length>0;
    }
    public Bitmap PrintDialog(IntPtr window) {
        var outer=Rect(window);
        Bounds client,visual;
        if(!GetClientRect(window,out client)) throw new Win32Exception();
        MapWindowPoints(window,IntPtr.Zero,ref client,2);
        var frame=outer;
        if(DwmGetWindowAttribute(window,9,out visual,Marshal.SizeOf(typeof(Bounds)))==0) {
            var painted=Rectangle.Intersect(outer,Rectangle.FromLTRB(visual.left,visual.top,visual.right,visual.bottom));
            if(!painted.IsEmpty) frame=painted;
        }
        var source=Rectangle.FromLTRB(frame.Left-outer.Left,frame.Top-outer.Top,frame.Right-outer.Left,frame.Bottom-outer.Top);
        // Some private desktops report the entire resize frame to DWM. PrintWindow leaves
        // its side and bottom gutters black. Inspect only the nonclient margins Windows owns.
        using(var pixels=Print(window)) {
            int left=source.Left,right=source.Right,bottom=source.Bottom;
            int y=client.top-outer.Top,height=client.bottom-client.top;
            while(left<client.left-outer.Left && EmptyFrameEdge(pixels,left,y,0,1,height)) left++;
            while(right>client.right-outer.Left && EmptyFrameEdge(pixels,right-1,y,0,1,height)) right--;
            while(bottom>client.bottom-outer.Top && EmptyFrameEdge(pixels,left,bottom-1,1,0,right-left)) bottom--;
            var visible=Rectangle.FromLTRB(left,source.Top,right,bottom);
            DialogBounds="window "+outer.Size+", DWM "+frame.Size+", captured "+visible;
            return pixels.Clone(visible,pixels.PixelFormat);
        }
    }
    // The Classic Modern skin defines a 62px thumb on a track starting at x=19
    // with a 17px right margin. Locate the light handle on the dark groove in PrintWindow.
    public double ClassicModernSeekFraction(Bitmap frame) {
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
    public double BentoSeekFraction(Bitmap frame) {
        // The silver thumb on Big Bento's 428px progress bar is 43px wide.
        for(int x=10;x<435;x++) {
            var pixel=frame.GetPixel(x,138);
            if(pixel.R>210 && pixel.G>210 && pixel.B>210) return (x-10)/385.0;
        }
        return -1;
    }
}
'@
Add-Type -ReferencedAssemblies $uiAssemblies -TypeDefinition ($controls+$nl+$capture)
Add-Type -AssemblyName System.Drawing

# Window controls, playback and capture
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
        default {if ($song.artist -eq 'Scapegoat Wax') {'Party'} else {'Flat'}}
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
function Set-CapturePosition($name,$slot,$only,$visualPreset) {
    $seconds=$seekPositions[$slot]
    [void](Wait-For {$ui.Send($main,0x400,0,105) -gt 0 -and $ui.Send($main,0x400,0,125) -eq $slot} 'decoder playback clock')
    $seekResult=$ui.Send($main,0x400,($seconds*1000),106)
    try {
        [void](Wait-For {
            $ms=$ui.Send($main,0x400,0,105)
            $ms -ge 1000*$seconds -and $ms -lt 1000*($seconds+2)
        } 'assigned playback position' 8)
    } catch {throw "Could not seek $name to ${seconds}s (result $seekResult, time $($ui.Send($main,0x400,0,105))ms, index $($ui.Send($main,0x400,0,125)), state $($ui.Send($main,0x400,0,104)), title $($ui.Text($main)))."}
    Start-Sleep -Milliseconds 250
    if ($visualizer -ne [IntPtr]::Zero -and $only -eq [IntPtr]::Zero) {
        [void](Wait-For {$ui.SpectrumMoving($ui.Root($visualizer))} "the $visualPreset spectrum to animate" 8)
    }
    return $seconds
}
function Get-ShotFrame($window) {
    if ($window -ne $layoutPlayer -or $stopped -or $classic) {return $ui.Print($window)}
    # Verify the thumb in the very bitmap that will be saved. A skin can repaint after a focus change.
    $duration=[double]$pose.queue[$slot].duration
    $timer=[Diagnostics.Stopwatch]::StartNew()
    do {
        $frame=$ui.Print($window)
        $shown=if ($skin -eq 'Big Bento Modern') {$ui.BentoSeekFraction($frame)} else {$ui.ClassicModernSeekFraction($frame)}
        $expected=$ui.Send($main,0x400,0,105)/(1000*$duration)
        if ([Math]::Abs($shown-$expected) -lt 0.025) {return $frame}
        $frame.Dispose()
        if ($timer.Elapsed.TotalSeconds -gt 8) {throw "Seek thumb for $name shows $shown, expected $expected."}
        Start-Sleep -Milliseconds 100
    } while ($true)
}
function Set-LibraryViewport($name,$slot) {
    if ($name -in 'classic-modern-album-tracks','classic-modern-playlist-tracks') {
        $form=Find-Form 1003
        $back=$ui.Item($form,1007)
        $extra=[Math]::Max(0,60-$ui.Rect($back).Width)
        if ($extra) {
            foreach ($id in 1007,1014,1003) { # Back, Search label, Search field
                $child=$ui.Item($form,$id)
                $box=$ui.ChildRect($form,$child)
                if ($id -ne 1007) {$box.X+=$extra}
                if ($id -eq 1007) {$box.Width+=$extra}
                if ($id -eq 1003) {$box.Width-=$extra}
                $ui.Place($child,$box.X,$box.Y,$box.Width,$box.Height)
            }
        }
    }
    $browser=Find-Form 1003
    if ($browser -and $ui.ControlText($ui.Item($browser,1003)).Length) {
        $list=$ui.Item($browser,1006)
        if ([CaptureWindows]::IsWindowVisible($ui.Item($browser,1043))) {$list=$ui.Item($browser,1043)}
        [void](Wait-For {$ui.Send($list,0x1004,0,0) -gt 1} "multiple visible search results for $name")
    }
    $section=switch ($name) {
        'classic-modern-genres' {'genre'}
        'classic-modern-album-artists' {'albumartist'}
        'classic-modern-artists' {'artist'}
        'classic-modern-tracks' {'song'}
        'classic-modern-playlists' {'playlist'}
        'classic-modern-playlist-tracks' {'playlistTracks'}
    }
    if ($section -and $pose.textWindows.$section) {
        $view=$pose.textWindows.$section
        $list=$ui.Item($browser,1006)
        try {[void](Wait-For {$ui.TopReportRow($list,$view.start,$view.total,$view.first)} "the $section viewport" 45)}
        catch {throw "$($_.Exception.Message) $($ui.RowStatus)"}
        Write-Log "$name shows $($ui.RowStatus)"
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
        if ($name -eq 'wacup-classic-albums' -and ($ui.CoversAcross -ne 4 -or $ui.FullCovers -lt 8)) {
            throw 'WACUP Classic must show four 150px covers across and at least two complete rows.'
        }
        if ($name -eq 'big-bento-modern-albums') {
            if ($ui.FullCovers -le 4) {throw 'Big Bento must show more than four complete album tiles.'}
            $panes=[Drawing.Rectangle[]]@([Drawing.Rectangle]::new(454,53,170,170),[Drawing.Rectangle]::new(825,377,312,312))
            $reference=Join-Path $run 'now-playing-cover.jpg'
            Get-DemoAsset 'getCoverArt' $pose.queue[$slot].coverArt $reference
            if (!$ui.ArtworkMatches($layoutPlayer,$reference,$panes)) {
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
                try {[void](Wait-For {$ui.ArtworkMatches($layoutPlayer,$reference,$panes)} 'both Big Bento playback covers' 15)}
                catch {throw "$($_.Exception.Message) $($ui.ArtworkStatus)"}
            }
        }
    }
    if ($browser) {
        foreach ($list in $ui.Windows($browser)) {
            if ($ui.Class($list) -ne 'SysListView32' -or ![CaptureWindows]::IsWindowVisible($list)) {continue}
            $columns=$ui.FitPopulatedColumns($list,@('Cache','Length','Tracks','Albums','Year','Disc','#'))
            if ($columns) {Write-Log "$name columns - $columns"}
        }
    }
}
function Save-WindowLayout($name,$playbackStatus) {
    [array]$frames=Get-Frames
    if (!$frames) {throw 'WACUP has no visible frames.'}
    [array]::Reverse($frames)
    $bounds = [Drawing.Rectangle]::Empty
    $boxes = @{}
    foreach ($window in $frames) {
        $rect = $ui.Rect($window)
        if ($placements.ContainsKey($window.ToInt64())) {
            $rect.Location = $placements[$window.ToInt64()]
        }
        $boxes[$window.ToInt64()]=$rect
        $bounds=if ($bounds.IsEmpty) {$rect} else {[Drawing.Rectangle]::Union($bounds,$rect)}
    }
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
    $canvas = New-Object Drawing.Bitmap $bounds.Width,$bounds.Height
    try {
        $g = [Drawing.Graphics]::FromImage($canvas)
        try {
            foreach ($window in $frames) {
                $rect = $boxes[$window.ToInt64()]
                $image = Get-ShotFrame $window
                $clip = $ui.Shape($window)
                try {
                    $x = $rect.X-$bounds.X; $y = $rect.Y-$bounds.Y
                    $clip.Translate($x,$y); $g.Clip=$clip
                    $g.DrawImageUnscaled($image,$x,$y)
                } finally { $clip.Dispose(); $image.Dispose() }
            }
        } finally { $g.Dispose() }
        $canvas.Save((Join-Path $shots ($name+'.png')),[Drawing.Imaging.ImageFormat]::Png)
        Write-Log ("Captured $name ("+$bounds.Width+' x '+$bounds.Height+') '+$playbackStatus)
    } finally { $canvas.Dispose() }

}
function Save-Shot($name,$only=[IntPtr]::Zero,$focusField=$tree,[switch]$stopped) {
    $slot=[array]::IndexOf($shotOrder,$name)
    if (!$stopped) {
        Start-PoseTrack $name
        $preset=Set-TrackEqualizer $pose.queue[$slot]
        if ($visualizer -ne [IntPtr]::Zero -and $only -eq [IntPtr]::Zero) {
            $visualPreset=$visualPresets[$slot%$visualPresets.Count]
            Set-AnalyzerPreset $visualizer $visualPreset
        }
    }
    if ($only -eq [IntPtr]::Zero) {Set-LibraryViewport $name $slot}
    $focusFrame=$ui.Root($focusField)
    if ($only -ne [IntPtr]::Zero -and $focusFrame -ne $only) {throw "Unexpected focus target for $name."}
    $ui.FocusFrame($focusFrame,$focusField)
    # Let the skin return from its temporary EQ adjustment text to the song title.
    if (!$stopped) {Start-Sleep -Milliseconds 1800}
    if ($only -eq [IntPtr]::Zero) {
        [void](Wait-For {
            $unexpected = @(Get-Frames | Where-Object {!$placements.ContainsKey($_.ToInt64())})
            return $unexpected.Count -eq 0
        } 'WACUP track notification to close' 20)
    }
    if ($stopped) {
        if ($ui.Send($main,0x400,0,104) -ne 0 -or $ui.Send($main,0x400,0,124) -ne 0) {
            throw 'Welcome needs a stopped player and an empty playlist.'
        }
        $playbackStatus='stopped with an empty playlist'
    } else {
        $seconds=Set-CapturePosition $name $slot $only $visualPreset
        $playbackStatus='playing '+$pose.queue[$slot].title+' at '+$seconds+'s, EQ '+$preset
    }
    $ui.CheckFocus($focusFrame,$focusField)
    if ($only -ne [IntPtr]::Zero) {
        $dialog=$ui.PrintDialog($only)
        try {
            $dialog.Save((Join-Path $shots ($name+'.png')),[Drawing.Imaging.ImageFormat]::Png)
            Write-Log "Captured $name ($($ui.DialogBounds)) $playbackStatus"
        } finally {$dialog.Dispose()}
        return
    }
    Save-WindowLayout $name $playbackStatus
}

function Get-Frame($title) {
    foreach ($h in Get-Frames) {
        if ($ui.Text($h) -eq $title) { return $h }
    }
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
        $ui.Post($main,0x111,40047,0)
        [void](Wait-For {$ui.Send($main,0x400,0,104) -eq 0} 'stopped before assembling the library playlist')
        [void]$ui.Send($main,0x400,0,101)
        # Saving the first server rebuilds the navigation tree. Wait for its actual
        # Tracks view before addressing the search field shared by every category.
        $view=Wait-For {
            if (!$ui.SelectTree($tree,'Tangerine Stream/Tracks')) {return}
            $candidate=Find-Form 1003
            if ($candidate -and $ui.Send($ui.Item($candidate,1006),0x1004,0,0) -eq $pose.textWindows.song.total) {return $candidate}
        } 'the connected Tracks view' 150
        Command $view 1051
        for ($i=0;$i -lt $pose.queue.Count;++$i) {
            $song=$pose.queue[$i]
            $ui.SetText($ui.Item($view,1003),$song.title)
            Command $view 1004
            try {[void](Wait-For {$ui.SelectListItem($ui.Item($view,1006),$song.title)} ('queue track '+$song.title))}
            catch {throw "$($_.Exception.Message) Search field [$($ui.ControlText($ui.Item($view,1003)))], status [$($ui.ControlText($ui.Item($view,1013)))]."}
            Command $view 1010
            [void](Wait-For {$ui.Send($main,0x400,0,124) -eq $i+1} 'playlist enqueue')
            if ($shotOrder[$i] -eq 'classic-modern-offline-cache') {Command $view 1011}
        }
    }
    Select-View 'Albums'
    $view=Wait-For {Find-Form 1003} 'album grid'
    $list=$ui.Item($view,1006)
    if ($skin -eq 'WACUP Classic') {
        Command $view 1050
        $editor=Wait-For {Find-Form 1060} 'the custom cover size editor'
        $ui.SetText($ui.Item($editor,1059),'150')
        Command $editor 1
        [void](Wait-For {!(Find-Form 1060)} 'the 150px WACUP Classic cover setting')
    } else {Command $view $(if ($skin -eq 'Big Bento Modern') {1601} else {1602})}
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
        if ($skin -eq 'Winamp Classic Modern') {Connect-DemoServer}
        [void](Wait-Library)
        Set-DemoQueue
        if ($skin -eq 'Winamp Classic Modern') {
            Save-ClassicModernGallery
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
        $shotOrder=@('add-server','classic-modern-albums','classic-modern-album-search',
            'classic-modern-album-tracks','classic-modern-genres','classic-modern-album-artists','classic-modern-artists',
            'classic-modern-tracks','classic-modern-playlists')
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
        $captured=@(Get-ChildItem -LiteralPath $shots -File -Filter '*.png' | ForEach-Object BaseName)
        $expected=@('classic-modern-welcome')+$shotOrder
        if (Compare-Object $expected $captured) {throw 'The captured PNGs do not match the requested gallery.'}
    } finally {
        if (!$server.HasExited) {$server.Kill(); $server.WaitForExit()}
        $server.Dispose()
    }
}
try {
    if ($action -eq 'refresh') {Invoke-Refresh} else {Invoke-Capture}
    [IO.File]::WriteAllText((Join-Path $run 'complete'),'ok')
} catch {Write-Log ($_.Exception.Message+$nl+$_.ScriptStackTrace);exit 1}
